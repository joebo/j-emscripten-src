/* ----------------------------------------------------------------------- */
/* J-Source Version 7 - COPYRIGHT 1993 Iverson Software Inc.               */
/* 33 Major Street, Toronto, Ontario, Canada, M5S 2K9, (416) 925 6096      */
/*                                                                         */
/* J-Source is provided "as is" without warranty of any kind.              */
/*                                                                         */
/* J-Source Version 7 license agreement:  You may use, copy, and           */
/* modify the source.  You have a non-exclusive, royalty-free right        */
/* to redistribute source and executable files.                            */
/* ----------------------------------------------------------------------- */
/*                                                                         */
/* Debug facilities		                                                   */

#include "j.h"
#include "x.h"
#include "p.h"
#include "d.h"

/* redefine GA to do correct cast */
#undef GA
#define GA(v,t,n,r,s)   RZ(v=(DC)ga((I)(t),(I)(n),(I)(r),(I*)(s)))

#define SUSNON 0      /* susact: no op                        */
#define SUSRUN 1      /* susact: run again                    */
#define SUSRET 2      /* susact: return result                */
#define SUSPOP 3      /* susact: signal result error          */
#define SUSRES 4      /* susact: reset stack                  */
#define SUSNXT 5      /* susact: run next line                */

static B debugbb;     /* empty si stack debugb setting        */
static B nflag;       /* 1 if space required before name      */
static A qstops;      /* stops set by the user                */
static C susact;      /* action on suspension end             */
static B suspend;     /* 1 if stay in current suspension      */

B        debugb;      /* debug as standard state for testing  */
B        deresetf;    /* dbr flag; 1 if suspension is enabled */
B        drun;        /* do not stop after run                */
A        qpopres;     /* result to pop to the next level      */
DC       sitop;       /* top of SI stack                      */

static void dhead(){if(outfile)fputc(COUT,outfile); jputc(qbx[9]);}
     /* preface stack display line */
     
static void dheadp(){dhead(); jputs("   ");}
     /* preface stack display line */

static void dwr(w)A w;{if(w){C*p=(C*)AV(w); DO(AN(w), jputc(p[i]););}}

static void dwrq(w)A w;{
 if(all1(match(alp,w)))jputs(nflag?" a.":"a.");
 else{C q=CQUOTE;
  jputc(q);
  if(w){C*p=(C*)AV(w); DO(AN(w), if(q==p[i])jputc(q); jputc(p[i]););}
  jputc(q);
}}

static void dname(w)A w;{C c=*(C*)AV(w);
 if(c==CGOTO)jputs("$.");
 else{
  if(nflag)jputc(' ');
  if(c==CALPHA)jputs("x.");
  else if(c==COMEGA)jputs("y.");
  else dwr(w);
}}

static void dspell(id)C id;{C c,s[3];
 s[2]=0;
 spellit(id,s);
 c=s[0]; if(c==CESC1||c==CESC2||nflag&&CA==ctype[c])jputc(' ');
 jputs(s);
}

static void disp(w)A w;{C err;I t;
 t=AT(w);
 switch(t){
  case BOOL:
  case INT:
  case FL:
  case CMPX:
   if(nflag)jputc(' ');
   err=jerr; jerr=0; w=thorn1(w); jerr=err;
   if(w)dwr(w); else jputs(" (ws full in numeric display) ");
   break;
  case NAME: dname(w);   break;
  case CHAR: dwrq(w);    break;
  case LPAR: jputc('('); break;
  case RPAR: jputc(')'); break;
  case ASGN: jputs(*AV(w)?"=.":"=:"); break;
  case MARK: break;
  default:   dspell(VAV(w)->id);
 }
 nflag=t&NAME+NUMERIC?1:0;
}

static DCF(seedebug){A t=*(si->ln+(A*)AV(qevm)); jputs(AV(t));}
     /* display error text */

static DCF(seedefn){dwr(si->p); JSPRX(1==si->n?"[%ld]":"[:%ld]",si->ln);}
     /* display function line */
     
static DCF(seeparse){A*s;I m,n,*sp;
 n=si->ln; /* # of tokens in token array */
 m=si->n;
 s=DSZX+(A*)si;
 sp=n+(I*)s; /* address i.#tokens to track location for error */
 m=*(sp+m);
 nflag=0; DO(n, if(i==m)jputs("  "); disp(s[i]););
}    /* display error line */

static DCF(debdisp){
 if(si){
  switch(si->t){
   case DCPARS: dheadp(); seeparse(si);            break;
   case DCDEFN: dhead();  seedefn(si);             break;
   case DCSCRP: dhead();  JSPRX("[-%ld]", si->ln); break;
   case DCDEBG: dhead();  seedebug(si);            break;
   case DCNAME: dhead();  dwr(si->m);              break;
  }
  jputc(CNL);
}}

static DCF(dispsis){
 while(si){
  if(DCDEBG==si->t) R;
  debdisp(si);
  if(DCDEFN==si->t || DCSCRP==si->t) R;
  si=si->lnk;
}}

static DCF(dispsi){while(si){debdisp(si); si=si->lnk;}}

I*deba(stack,i)A*stack;I i;{DC d; I k,*sp,*spx;
 GA(d,INT,2*i+DSZ,1L,0);
 d->t=DCPARS;
 d->lnk=sitop;
 sitop=d;    /* linkx new debug token array to top of si stack */
 d->ln=i;    /* tokens in debug token array                   */
 d->n=i-1;   /* assumed error at end of token array           */
 sp=DSZX+(I*)d;
 memcpy(sp,stack,i*sizeof(A*));
 spx=sp=i+sp;
 k=0;
 while(i--)*spx++=k++;
 R sp;
}

DC debadd(type)I type;{DC d;
 GA(d,INT,DSZ,1,0);
 d->t=type;
 d->lnk=sitop; sitop=d; /* linkx new debug stack entry to top of si */
 R d;
}

void debz(){
 if( sitop)sitop=sitop->lnk;
 if(!sitop){debugb=debugbb; deresetf=0;}
}

static void susp(){I old=tbase+ttop;C*sp;
 suspend=debugb;
 scad=0;
 while(suspend){
  immex(jgets("      ")); jerr=0;
  tpop(old);
 }
 suspend=debugb;
}

void debug(){A name,seq;C err,*sp,*spz,*spp;DC si;
 sp=scad; spz=sczad; spp=scpad; si=sitop;
 if(!debugb||!si) R;
 if(DCPARS==si->t) si=si->lnk;
 if(!si || DCDEFN!=si->t && DCSCRP!=si->t) R;
 err=jerr; jerr=0;
 if(!debadd(DCDEBG)){debugb=0; jsignal(EVSYSTEM); R;}
 sitop->ln=err;
 if(DCDEFN==si->t){
  name=scnm(CGOTO);
  seq=srd(name,local);
  seq=over(sc(si->ln),seq);
  symbis(name,seq,local);
  susp();
  scad=sp; sczad=spz; scpad=spp;
  switch(susact){
   case SUSNON: break;
   case SUSRUN: break;
   case SUSRET: symbis(name, mtv, local); break;
   case SUSPOP: symbis(name, mtv, local); break;
   case SUSRES: break;
   case SUSNXT: symbis(name, behead(seq), local); break;
 }} else if(DCSCRP==si->t){
  susp();
  scad=sp; sczad=spz; scpad=spp;
  switch(susact){
   case SUSNON: break;
   case SUSRUN: scad=scpad; break;
   case SUSRET: fa(qpopres); qpopres=0;
   case SUSPOP: scad=0; break;
   case SUSRES: break;
   case SUSNXT: break;
 }}
 susact=SUSNON; debz();
}

static B stopsub(C *p, C *nw, I md){C c,ca,*cp,*sp; I n;
	sp=strchr(p,';');
	if(!sp) sp=p+strlen(p);
	cp=strchr(p,':');
	if((!cp)||(cp>sp)) cp=sp;
	if(2==md){p=cp; cp=sp;}
	sp=strchr(p, '*');
	if(sp && sp<=cp) R 1;
	n=strlen(nw);
	while(p<cp){
		if(!strncmp(p, nw, n)){
			ca=*(p-1); c=*(p+n); 
			if((c==0||c==' '||c==':'||c==';')&&(ca==' '||ca==':'))	R 1;
		}
		p++;
	}
	R 0;
}
	
/* check for stop before each function line; return 1 if stop requested */
/* 0 or more repetitions of the following pattern, separated by ;       */
/* f m:d   function name; monad line #s; dyad line #s.  * means all     */
B dbcheck(){A t;C nw[10],*s,*tv;DC dv;I md,tn;
 if(!qstops)R 0;
 if(!sitop->lnk)R 0;
 dv=sitop->lnk;
 if(DCDEFN!=dv->t)R 0;
 if(drun){drun=0; R 0;}
 t=dv->p;
 if(!t)R 0;
 tn=AN(t); tv=(C*)AV(t); s=(C*)AV(qstops); md=dv->n; sprintf(nw,"%i",dv->ln); 
 while(s){
  while(' '==*s)++s;
  if('*'==*s){s++; if(stopsub(s,nw,md))R 1;}
  else if(!strncmp(s,tv,tn)){s+=tn; if(' '==*s&&stopsub(s,nw,md))R 1;}
  s=strchr(s,';'); if(s)++s;
 }
 R 0;
}

F1(dbr){I k;
 if(AN(w)){RE(k=i0(w)); ASSERT(0==k||1==k,EVDOMAIN); debugbb=k;}
 deresetf=1; debugb=suspend=0; susact=SUSRES; 
 R mtm;
}    /* 13!:0  reset stack; enable/disable suspension */

F1(dbs){DC si=sitop;
 RZ(w);
 if(si&&si->t==DCNAME) si=si->lnk;
 if(si&&si->t==DCPARS) si=si->lnk;
 dispsi(si); 
 R mtm;
}    /* 13!:1  display SI stack */

F1(dbsq){RZ(w); R qstops?qstops:mtv;}
     /* 13!:2  query stops */

F1(dbss){
 RZ(w);
 ASSERT(1==AR(w),EVRANK);
 ASSERT(CHAR&AT(w),EVDOMAIN);
 if(AN(w))ra(w); fa(qstops); qstops=AN(w)?w:0;
 R mtv;
}    /* 13!:3  set stops */

F1(dbrun){RZ(w); suspend=0; drun=1; susact=SUSRUN; R mtm;}
     /* 13!:4  run again */

F1(dbnxt){RZ(w); suspend=0; drun=1; susact=SUSNXT; R mtm;}
     /* 13!:5  run next */

F1(dbret){
 RZ(w); 
 suspend=0; drun=1; susact=SUSRET; 
 fa(qpopres); qpopres=0; qpopres=ra(w); 
 R mtm;
}    /* 13!:6  exit and signal result error */

F1(dbpop){RZ(w); suspend=0; drun=1; susact=SUSPOP; R mtm;}
     /* 13!:7  exit and return v */

     
static void jsig(){
 tostdout=1; suspend=1;
 if(debugb&&!spc()){
  dhead(); jputs("ws full (can not debug suspend)"); jputc(CNL);
  debugb=0;
 }
 dhead(); jputs(AV(*(jerr+(A*)AV(qevm)))); jputc(CNL);
}

static void jsigz(){
 if(debugb)dispsis(sitop); else{dispsi(sitop); deresetf=0!=sitop;}
}

void jsignal(e)int e;{
 if(jerr)R; jerr=e; if(deresetf||!errsee) R;
 jsig(); jsigz();
}

void jsignalx(e,w,n)int e;A w;I n;{
 if(jerr)R; jerr=e; if(deresetf||!errsee) R;
 jsig(); 
 dheadp(); dwr(w); jputc(CNL);
 if(n!=-1){dheadp(); DO(n, jputc(' ');); jputc('^'); jputc(CNL);}
 jsigz();
}

