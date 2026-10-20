main:
	cmake -S . -B build
	cmake --build build
g:
	git add -A ; git commit -m "sync" ; git push
p:
	git pull
m:
	nvim makefile
