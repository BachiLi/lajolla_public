uint32_t register_embree_op::operator()(const TriangleMesh &mesh) const {
    RTCGeometry rtc_geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    // A geomID is the ID associated with the shape inside Embree.
    uint32_t geomID = rtcAttachGeometry(scene, rtc_geom);
    Vector4f *positions = (Vector4f*)rtcSetNewGeometryBuffer(
        rtc_geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3,
        sizeof(Vector4f), mesh.positions.size());
    Vector3i *triangles = (Vector3i*)rtcSetNewGeometryBuffer(
        rtc_geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
        sizeof(Vector3i), mesh.indices.size());
    for (int i = 0; i < (int)mesh.positions.size(); i++) {
        Vector3 position = mesh.positions[i];
        positions[i] = Vector4f{(float)position[0], (float)position[1], (float)position[2], 0.f};
    }
    for (int i = 0; i < (int)mesh.indices.size(); i++) {
        triangles[i] = mesh.indices[i];
    }
    rtcSetGeometryVertexAttributeCount(rtc_geom, 1);
    rtcCommitGeometry(rtc_geom);
    rtcReleaseGeometry(rtc_geom);
    return geomID;
}

PointAndNormal sample_point_on_shape_op::operator()(const TriangleMesh &mesh) const {
    int tri_id = sample(mesh.triangle_sampler, w);
    assert(tri_id >= 0 && tri_id < (int)mesh.indices.size());
    Vector3i index = mesh.indices[tri_id];
    Vector3 v0 = mesh.positions[index[0]];
    Vector3 v1 = mesh.positions[index[1]];
    Vector3 v2 = mesh.positions[index[2]];
    Vector3 e1 = v1 - v0;
    Vector3 e2 = v2 - v0;
    // https://pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations#SamplingaTriangle
    Real a = sqrt(std::clamp(uv[0], Real(0), Real(1)));
    Real b1 = 1 - a;
    Real b2 = a * uv[1];
    Vector3 geometric_normal = normalize(cross(e1, e2));
    // Flip the geometric normal to the same side as the shading normal
    if (mesh.normals.size() > 0) {
        Vector3 n0 = mesh.normals[index[0]];
        Vector3 n1 = mesh.normals[index[1]];
        Vector3 n2 = mesh.normals[index[2]];
        Vector3 shading_normal = normalize(
            (1 - b1 - b2) * n0 + b1 * n1 + b2 * n2);
        if (dot(geometric_normal, shading_normal) < 0) {
            geometric_normal = -geometric_normal;
        }
    }
    return PointAndNormal{v0 + (e1 * b1) + (e2 * b2), geometric_normal};
}

Real surface_area_op::operator()(const TriangleMesh &mesh) const {
    return mesh.total_area;
}

Real pdf_point_on_shape_op::operator()(const TriangleMesh &mesh) const {
    return 1 / surface_area_op{}(mesh);
}

void init_sampling_dist_op::operator()(TriangleMesh &mesh) const {
    std::vector<Real> tri_areas(mesh.indices.size(), Real(0));
    Real total_area = 0;
    for (int tri_id = 0; tri_id < (int)mesh.indices.size(); tri_id++) {
        Vector3i index = mesh.indices[tri_id];
        Vector3 v0 = mesh.positions[index[0]];
        Vector3 v1 = mesh.positions[index[1]];
        Vector3 v2 = mesh.positions[index[2]];
        Vector3 e1 = v1 - v0;
        Vector3 e2 = v2 - v0;
        tri_areas[tri_id] = length(cross(e1, e2)) / 2;
        total_area += tri_areas[tri_id];
    }
    mesh.triangle_sampler = make_table_dist_1d(tri_areas);
    mesh.total_area = total_area;
}

ShadingInfo compute_shading_info_op::operator()(const TriangleMesh &mesh) const {
    // Get UVs of the three vertices
    assert(vertex.primitive_id >= 0);
    Vector3i index = mesh.indices[vertex.primitive_id];
    Vector2 uvs[3];
    if (mesh.uvs.size() > 0) {
        uvs[0] = mesh.uvs[index[0]];
        uvs[1] = mesh.uvs[index[1]];
        uvs[2] = mesh.uvs[index[2]];
    } else {
        // Use barycentric coordinates
        uvs[0] = Vector2{0, 0};
        uvs[1] = Vector2{1, 0};
        uvs[2] = Vector2{1, 1};
    }
    // Barycentric coordinates are stored in vertex.st
    Vector2 uv = (1 - vertex.st[0] - vertex.st[1]) * uvs[0] +
                 vertex.st[0] * uvs[1] +
                 vertex.st[1] * uvs[2];
    Vector3 p0 = mesh.positions[index[0]],
            p1 = mesh.positions[index[1]],
            p2 = mesh.positions[index[2]];
    // We want to derive dp/du & dp/dv. We have the following
    // relation:
    // p  = (1 - s - t) * p0   + s * p1   + t * p2
    // uv = (1 - s - t) * uvs0 + s * uvs1 + t * uvs2
    // dp/duv = dp/dst * dst/duv = dp/dst * (duv/dst)^{-1}
    // where dp/dst is a 3x2 matrix, duv/dst and dst/duv is a 2x2 matrix,
    // and dp/duv is a 3x2 matrix.

    // Let's build duv/dst first. To be clearer, it is
    // [du/ds, du/dt]
    // [dv/ds, dv/dt]
    Vector2 duvds = uvs[2] - uvs[0];
    Vector2 duvdt = uvs[2] - uvs[1];
    // The inverse of this matrix is
    // (1/det) [ dv/dt, -du/dt]
    //         [-dv/ds,  du/ds]
    // where det = duds * dvdt - dudt * dvds
    Real det = duvds[0] * duvdt[1] - duvdt[0] * duvds[1];
    Real dsdu =  duvdt[1] / det;
    Real dtdu = -duvds[1] / det;
    Real dsdv =  duvdt[0] / det;
    Real dtdv = -duvds[0] / det;
    Vector3 dpdu, dpdv;
    if (fabs(det) > 1e-8f) {
        // Now we just need to do the matrix multiplication
        Vector3 dpds = p2 - p0;
        Vector3 dpdt = p2 - p1;
        dpdu = dpds * dsdu + dpdt * dtdu;
        dpdv = dpds * dsdv + dpdt * dtdv;
    } else {
        // degenerate uvs. Use an arbitrary coordinate system
        std::tie(dpdu, dpdv) = coordinate_system(vertex.geometric_normal);
    }

    // Now let's get the shading normal & mean_curvature.
    // By default it is the geometry normal and we have zero curvature.
    Vector3 shading_normal = vertex.geometric_normal;
    Real mean_curvature = 0;
    Vector3 tangent, bitangent;
    // However if we have vertex normals, that overrides the geometry normal.
    if (mesh.normals.size() > 0) {
        Vector3 n0 = mesh.normals[index[0]],
                n1 = mesh.normals[index[1]],
                n2 = mesh.normals[index[2]];
        shading_normal = normalize(
            (1 - vertex.st[0] - vertex.st[1]) * n0 + 
                                vertex.st[0] * n1 +
                                vertex.st[1] * n2);
        // dpdu may not be orthogonal to shading normal:
        // subtract the projection of shading_normal onto dpdu to make them orthogonal
        tangent = normalize(dpdu - shading_normal * dot(shading_normal, dpdu));

        // We want to compute dn/du & dn/dv for mean curvature.
        // This is computed in a similar way to dpdu.
        // dn/duv = dn/dst * dst/duv = dn/dst * (duv/dst)^{-1}
        Vector3 dnds = n2 - n0;
        Vector3 dndt = n2 - n1;
        Vector3 dndu = dnds * dsdu + dndt * dtdu;
        Vector3 dndv = dnds * dsdv + dndt * dtdv;
        bitangent = normalize(cross(shading_normal, tangent));
        mean_curvature = (dot(dndu, tangent) + 
                          dot(dndv, bitangent)) / Real(2);
    } else {
        tangent = normalize(dpdu - shading_normal * dot(shading_normal, dpdu));
        bitangent = normalize(cross(shading_normal, tangent));
    }

    Frame shading_frame(tangent, bitangent, shading_normal);
    return ShadingInfo{uv, shading_frame, mean_curvature,
                       max(length(dpdu), length(dpdv)) /* inv_uv_size */};
}
