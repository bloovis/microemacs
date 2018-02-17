microemacs.pdf : microemacs.md
	pandoc -s --template=template.latex -o microemacs.tex microemacs.md
	latex microemacs.tex
	latex -output-format=pdf microemacs.tex

.PHONY : clean

clean :
	-rm microemacs.log microemacs.pdf microemacs.toc microemacs.aux microemacs.out \
		microemacs.ilg microemacs.ind microemacs.idx microemacs.dvi microemacs.tex
