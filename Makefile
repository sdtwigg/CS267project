UPCC = cc -h upc -O

TARGETS=project
CFILES = project.c common.c spinlock.c limited_directory.c write_list.c read_list.c read_tree.c

all: $(TARGETS)

project: project.h $(CFILES)
	$(UPCC) -o $@ $(CFILES)

clean:
	rm -f *.o $(TARGETS)
