LATEX := pdflatex -halt-on-error

FIGURES =
FIGURES += figures/rmw_atomicity2.ps

#FIGHEAD = #figures/figHeader.dotpiece
#FIGFOOT = #figures/figFooter.dotpiece


.SUFFIXES : .tex .bbl .aux .eps .ps .dot .pdf

.tex.bbl:
	bibtex $*

.tex.aux:
	latex $*

%.eps: %.ps
	ps2epsi $< $@

%.ps: %.dot
	dot -Tps $< -o $@

%.pdf: %.dot
	dot -Tpdf $< -o $@

%.png: %.dot
	dot -Tpng $< -o $@

%.aux: %.tex
	$(LATEX) $<

#%.dot: %.dotpiece $(FIGHEAD) $(FIGFOOT)
#	cat $(FIGHEAD) $< $(FIGFOOT) > $@



# latex the paper (default)
new:	biblinks newpaper

# latex + bibtex the paper
bib:	bibpaper newpaper

# save a backup of tex files in BACKUPS
bak: 	backuppaper


biblinks: figures
	$(LATEX) paper.tex
	bibtex paper.aux
	$(LATEX) paper.tex


newpaper: figures biblinks
	$(LATEX) paper.tex


bibpaper: paper.aux paper.bbl
	latex paper.tex


backuppaper: 
	@if [ ! -d BACKUPS ]; then mkdir BACKUPS; fi;	\
	crtdate=`date '+%m-%d-%y--%H:%M:%S'`;		\
	bakdir=BACKUPS/$$crtdate;			\
	mkdir $$bakdir;					\
	cp *.tex $$bakdir

total:
	latex paper.tex
	bibtex paper.aux
	latex paper.tex
	latex paper.tex
	pdflatex paper.tex


figures: $(FIGURES)


errinjfig:
	gnuplot err-inj-fig/nve2latex.cmds
	epstopdf injectErrorFig.eps
	mv injectErrorFig.* figures


clean:
	rm -f *.dvi *.log *.aux *.blg *.bbl *~ err-inj-fig/*~
	rm -f paper.ps paper.pdf 
	rm -f injectErrorFig*
