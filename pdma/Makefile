CC = gcc
INCLUDE = .
CFLAGS = -I$(INCLUDE) -Wall -Wpedantic
#LIBS = -ldmalloc -lm
LIBS = -lm

MD2HTML = pandoc
MD2PDF = pandoc --pdf-engine=xelatex

MDFILES=$(wildcard *.md)

HTMLFILES=$(patsubst %.md,%.html,$(MDFILES))
PDFFILES=$(patsubst %.md,%.pdf,$(MDFILES))

DEPS = pdma-ex.c mss_pdma.c mss_pdma.c 
OBJS = pdma-ex.o mss_pdma.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

pdma-ex: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

html: $(HTMLFILES)
pdf: $(PDFFILES)

%.html: %.md
	$(MD2HTML) -o $@ $<

%.pdf: %.md
	$(MD2PDF) -o $@ $<

.PHONY: clean

clean:
	rm -rf *.o *~ core pdma-ex *.html *.pdf

