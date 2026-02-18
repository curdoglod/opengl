.PHONY: all game clean clean_engine clean_game clean_all re run

all:
	cd Engine && $(MAKE)
	cd Game   && $(MAKE)
game: 
	cd Game && $(MAKE)

clean_engine:
	cd Engine && $(MAKE) clean

clean_game:
	cd Game && $(MAKE) clean

clean: clean_game

clean_all: clean_engine clean_game

re: clean_all all

run: all
	./game_app
