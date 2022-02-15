.PHONY: FORCE

all: pdf/homework0.pdf pdf/homework1.pdf pdf/homework2.pdf pdf/homework3.pdf

pdf/homework0.pdf: FORCE
	mkdir -p pdf
	./latexrun homework0.tex
	mv homework0.pdf pdf/homework0.pdf

pdf/homework1.pdf: FORCE
	mkdir -p pdf
	./latexrun homework1.tex
	mv homework1.pdf pdf/homework1.pdf

pdf/homework2.pdf: FORCE
	mkdir -p pdf
	./latexrun homework2.tex
	mv homework2.pdf pdf/homework2.pdf

pdf/homework3.pdf: FORCE
	mkdir -p pdf
	./latexrun homework3.tex
	mv homework3.pdf pdf/homework3.pdf

.PHONY: clean
clean:
	./latexrun --clean-all
