main:
	pdflatex hmwk.tex
o:
	nvim hmwk.tex
r:
	open hmwk.pdf
c:
	rm hmwk*
g:
	git add -A ; git commit -m "sync" ; git push
p:
	git pull
m:
	nvim makefile
