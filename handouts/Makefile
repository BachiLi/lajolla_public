.PHONY: FORCE
pdf/homework0.pdf: FORCE
	mkdir -p pdf
	./latexrun homework0.tex
	mv homework0.pdf pdf/homework0.pdf

.PHONY: clean
clean:
	./latexrun --clean-all
