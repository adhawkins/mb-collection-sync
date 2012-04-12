CXXFLAGS=-Wall -Werror `neon-config --cflags`
CXXFLAGS+=-g -ggdb -O0

# To enable tracing into the libraries and also expose some more
# obscure bugs during development:
#CXXFLAGS+=-D_GLIBCXX_DEBUG

OBJS=mb-collection-sync.o CollectionSync.o

SRCS=$(OBJS:.o=.cc)

all: mb-collection-sync

clean:
	rm -f $(OBJS) mb-collection-sync svn-commit.* *.d

%.d: %.cc
	@echo DEPEND $< $@
	@$(CXX) -MM $(CXXFLAGS) $< | \
        sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@

mb-collection-sync: $(OBJS)
	g++ `neon-config --libs` -o $@ -lFLAC++ -lmusicbrainz4 $(OBJS) -L../libmusicbrainz-restructure/src

ifneq "$(MAKECMDGOALS)" "clean"
-include $(SRCS:.cc=.d)
endif

.phony:
