
all: hummingbird_logo.rle ovation_logo.rle

hummingbird_logo.rle:
	convert $(patsubst %.rle,%.bmp,$@) rgb:$(patsubst %.rle,%.raw,$@)
	rgb2565 -rle < $(patsubst %.rle,%.raw,$@) > $@
	$(shell identify -format "%[fx:w]\n%[fx:h]\n" $(patsubst %.rle,%.bmp,$@) > $@.tmp)
	cat $@ >> $@.tmp
	mv $@.tmp $@

ovation_logo.rle:
	convert $(patsubst %.rle,%.bmp,$@) rgb:$(patsubst %.rle,%.raw,$@)
	rgb2565 -rle < $(patsubst %.rle,%.raw,$@) > $@
	$(shell identify -format "%[fx:w]\n%[fx:h]\n" $(patsubst %.rle,%.bmp,$@) > $@.tmp)
	cat $@ >> $@.tmp
	mv $@.tmp $@

install:
	mv hummingbird_logo.rle ../../board/bn_hummingbird/
	mv ovation_logo.rle ../../board/bn_ovation/

clean:
	$(RM) *.rle *.raw


