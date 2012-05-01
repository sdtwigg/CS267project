UPCC = cc -h upc -O

TARGETS=project
CFILES = project.c common.c spinlock.c limiteddirectory.c

all: $(TARGETS)

project: project.h $(CFILES)
	$(UPCC) -o $@ $(CFILES)

clean:
	rm -f *.o $(TARGETS)