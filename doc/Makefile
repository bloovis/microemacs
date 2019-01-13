microemacs.pdf : microemacs.md template.latex
	pandoc -s --template=template.latex -o microemacs.tex microemacs.md
	latex microemacs.tex
	latex -output-format=pdf microemacs.tex

# The imakeidx package must be loaded before the hyperref package.
# This rule modifies Pandoc's latex template to allow that to happen.
# Then the following lines must be added to the preamable of the Markdown file:
#   before-hyperref:
#       - \usepackage{imakeidx}

DEFAULT_TEMPLATE = /usr/share/pandoc/data/templates/default.latex
template.latex : ${DEFAULT_TEMPLATE}
	sed -e 's/\\usepackage{hyperref}/$$for(before-hyperref)$$\n$$before-hyperref$$\n$$endfor$$\n&/' \
		${DEFAULT_TEMPLATE} >template.latex

.PHONY : clean

clean :
	-rm microemacs.log microemacs.pdf microemacs.toc microemacs.aux microemacs.out \
		microemacs.ilg microemacs.ind microemacs.idx microemacs.dvi microemacs.tex
