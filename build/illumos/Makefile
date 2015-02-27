include ../Makefile.master

LIBS+=			-lumem
CFLAGS+=		-D UMEM
UMEM_CFLAGS=		$(CFLAGS) "-Wno-unused-parameter"

GRAPH_PROV=	graph_provider.d

OBJECTS=	$(C_OBJECTS) $(D_OBJECTS)


$(D_HDRS): 
	$(DTRACEH) $(SRCDIR)/$(D_SCR) -o $(SRCDIR)/$(D_HDRS)

#
# This is used for debugging (like expanding variables), because Make sucks at
# error messages.
#
fuckit: 
	echo $(DRV_SRCS)



#.PHONY: $(C_SRCS)

$(C_SRCS): %.c: $(D_HDRS)

$(C_OBJECTS): %.o: %.c $(C_HDRS)
	$(CC) $(CFLAGS) $(CINC) -o $@ -c $<
	$(CTFC_POSTPROC)
#$(CTFCONVERT) -i -L VERSION $@

objs: $(OBJECTS)

$(D_OBJECTS): $(C_OBJECTS)
	$(DTRACEG) $(SRCDIR)/$(GRAPH_PROV) $(C_OBJECTS)

$(SO): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(SO) $(OBJECTS) $(LIBS)
	$(CTFM_POSTPROC)

all: $(SO)

$(DRV): $(DRV_SRCS)
	gcc -o $@ $< $(DCINC) $(DLDFLAGS) $(DLIBS)


# $(CTFMERGE) -L VERSION -o $@ $(C_OBJECTS)


uninstall:
	pfexec rm -r $(PREFIX) 2> /dev/null

install: 
	-pfexec rm -r $(PREFIX) 2> /dev/null
	-pfexec mkdir $(PREFIX)
	-pfexec mkdir $(PREFIX)/lib/
	-pfexec mkdir $(PREFIX)lib/64
	pfexec cp $(SO) $(PREFIX)/lib/64/
	pfexec ln -s $(PREFIX)/lib/64/$(SO) $(PREFIX)/lib/64/libgraph.so
	pfexec mkdir $(PREFIX)/include
	-pfexec mkdir $(PREFIX)/include/dtrace
	pfexec cp $(SRCDIR)/graph.h $(PREFIX)/include/
	-pfexec cp $(SRCDIR)/graph.d $(PREFIX)/include/dtrace/



$(PLISTS): %.plist: %.c $(D_HDRS)
	$(CKSTATIC) -D UMEM $< -o $@

$(CSTYLES): %.cstyle: %.c
	$(CSTYLE) $<

.PHONY: cstyle
cstyle: $(CSTYLES)


$(R_BENCH): 
	mkdir $(R_BENCH)
	mkdir $(R_BENCH)/sl
	

$(R_BENCH_SD): $(R_BENCH)/%: %.Z $(R_BENCH)
	mkdir $@

$(SL_BENCH_R_I): $(DRV) $(R_BENCH_SD) 
	$(DTRACE) -c './$(DRV) sl $(BENCH_SIZE) intsrt rand' -s $(BENCH_SL_THR_HEAP) -o $@

$(SL_BENCH_S_I): $(DRV) $(R_BENCH_SD) 
	$(DTRACE) -c './$(DRV) sl $(BENCH_SIZE) intsrt seqinc' -s $(BENCH_SL_THR_HEAP) -o $@
	$(AWK) -f $(BENCH_PPROC) $(R_BENCH)/sl/$(DS_SI_SUF) > $(R_BENCH)/sl/$(DS_SI_PP_SUF)

bench: $(DRV) $(SL_BENCH) $(SL_BENCH_PP)

bench_plot: 
	$(NAWK) $(NAWK_ARGS) -f $(BENCH_PLOT)/gen.nawk
	$(BENCH_PLOT)/plot.ksh

$(FGDIR):
	mkdir $(FGDIR)
	mkdir $(FGDIR)/time

$(FG_OUT): $(FGDIR) $(DRV)
	$(DTRACE) $(DTRACE_FRAMES) -c '$(FG_CMD)' -n $(value FG_TIME_STACKS) -o $@

$(FG_SVG): $(FG_OUT)
	$(STACKCOLLAPSE) $< > temp_folded
	$(FLAMEGRAPH) temp_folded > $@
	rm temp_folded



.PHONY: flamegraphs
flamegraphs: $(FG_SVG)

clean_flamegraphs:
	pfexec rm -r $(FGDIR)

clean_bench:
	pfexec rm -r $(R_BENCH)

clean_plot:
	pfexec rm -r imgs
	pfexec rm -r rcode

check: $(PLISTS)
	echo CHECK DONE

clean:
	rm $(SRCDIR)/$(D_HDRS)
	rm $(OBJECTS)
	rm $(SO)

clean_cstyle:
	rm $(SRCDIR)/$(D_HDRS)

clean_check:
	rm $(PLISTS)

clean_drv:
	rm $(DRV)
