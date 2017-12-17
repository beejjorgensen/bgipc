# to be included from singlepage/Makefile and multipage/Makefile

pipe1-96-4.149.png: $(IMGPATH)/pipe1-96-4.149.png
	cp $< $@

$(VALIDFILE): $(SRC)
	$(BINPATH)/bgvalidate $< $@

.PHONY: pristine
pristine: clean
	rm -f *.html $(IMGS) $(PACKAGE).css
	rm -f ../*.tgz ../*.zip

.PHONY: clean
clean:

