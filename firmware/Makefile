YOTTA=yt
DEST=dist/microle.hex

HEX=build/bbc-microbit-classic-gcc/source/microle-combined.hex

deploy: $(HEX)
	sudo cp $(HEX) $(DEST)

$(HEX): .yotta.json
	$(YOTTA) clean
	$(YOTTA) build

.yotta.json:
	$(YOTTA) target bbc-microbit-classic-gcc

.PHONY: $(HEX)

clean:
	$(YOTTA) clean
	rm dist/ -rf
	mkdir -p dist
	rm .yotta.json
