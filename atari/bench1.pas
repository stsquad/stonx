program bench1;
uses utilunit;

var t: longint;
    l: longint;
    
procedure bench_nop; assembler;
label foo, bar;
asm
  move #24,d1
  bar:
  move #999,d0
  foo:
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  dbf d0,foo
  dbf d1,bar
end;

procedure bench_moveq; assembler;
label foo, bar;
asm
  move #24,d1
  bar:
  move #999,d0
  foo:
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  moveq #42,d2
  dbf d0,foo
  dbf d1,bar
end;
            
procedure bench_regs; assembler;
label foo, bar;
asm
  move #24,d1
  bar:
  move #999,d0
  foo:
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2    
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2    
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2    
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2
    move d1,d2    
    dbf d0,foo
    dbf d1,bar
end;

procedure bench_lregs; assembler;
label foo, bar;
asm
  move #4,d1
  bar:
  move #9999,d0
  foo:
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    add.l d1,d2
    dbf d0,foo
    dbf d1,bar
end;

procedure bench_abs; assembler;
label foo, bar;
asm
  move #19999,d0
  foo:
    move.l l,l
    move.l l,l
    move.l l,l
    move.l l,l
    move.l l,l
    move.l l,l
    move.l l,l
    move.l l,l
    move.l l,l
    move.l l,l
    move.l l,l
    move.l l,l
    move.l l,l
    move.l l,l
    move.l l,l
    move.l l,l
    move.l l,l
    move.l l,l
    move.l l,l
    move.l l,l
    move.l l,l
    move.l l,l
    move.l l,l
    move.l l,l
    move.l l,l
    dbf d0,foo
end;

procedure bench_movem; assembler;
label foo;
var m: array [0..15] of longint;
asm;
  move #4999,d0
foo:
  movem.l m,d1-a5
  movem.l d1-a5,m
  movem.l m,d1-a5
  movem.l d1-a5,m
  movem.l m,d1-a5
  movem.l d1-a5,m
  movem.l m,d1-a5
  movem.l d1-a5,m
  movem.l m,d1-a5
  movem.l d1-a5,m
  movem.l m,d1-a5
  movem.l d1-a5,m
  movem.l m,d1-a5
  movem.l d1-a5,m
  movem.l m,d1-a5
  movem.l d1-a5,m
  movem.l m,d1-a5
  movem.l d1-a5,m
  movem.l m,d1-a5
  movem.l d1-a5,m
  dbf d0,foo
end;

procedure bench_stack; assembler;
label foo;
asm;
  move #24999,d0
foo:
  pea $12345678  
  move.l (sp)+,d1
  pea $12345678  
  move.l (sp)+,d1
  pea $12345678  
  move.l (sp)+,d1
  pea $12345678  
  move.l (sp)+,d1
  pea $12345678  
  move.l (sp)+,d1
  pea $12345678  
  move.l (sp)+,d1
  pea $12345678  
  move.l (sp)+,d1
  pea $12345678  
  move.l (sp)+,d1
  pea $12345678  
  move.l (sp)+,d1
  pea $12345678  
  move.l (sp)+,d1
  dbf d0,foo
end;

procedure bench_bsr; assembler;
label foo,l1,l2,l3,l4,l5,l6,l7,l8,l9,
      l10,l11,l12,l13,l14,l15,l16,l17,l18,l19,l20,done;
asm
  move #19999,d0
foo:
  bsr l1
  dbf d0,foo
  bra done
l1:
  bsr l2
  rts
l2:
  bsr l3
  rts
l3:
  bsr l4
  rts
l4:
  bsr l5
  rts
l5:
  bsr l6
  rts
l6:
  bsr l7
  rts
l7:
  bsr l8
  rts
l8:
  bsr l9
  rts
l9:
  bsr l10
  rts
l10:
  bsr l11
  rts
l11:
  bsr l12
  rts
l12:
  bsr l13
  rts
l13:
  bsr l14
  rts
l14:
  bsr l15
  rts
l15:
  bsr l16
  rts
l16:
  bsr l17
  rts
l17:
  bsr l18
  rts
l18:
  bsr l19
  rts
l19:
  bsr l20
  rts
l20:
  rts
  done:
end;

procedure time0;
begin
  t := hz200;
end;

procedure stamp (s: string; n: Longint);
var o: longint;
begin
  o:=hz200-t;
  writeln ('Time for ',n :7,' * ',s :15,': ', o*5 :4, ' ms; 8MHz cycles: ',
    trunc(40000*o)/n:6:1);
  t := hz200;
end;

var x: char;
begin
  time0;
  bench_nop; stamp ('nop',2500000);
  bench_moveq; stamp ('moveq',2500000);
  bench_regs; stamp ('move d1,d2',2500000);
  bench_lregs; stamp ('add.l d1,d2',2500000);
  bench_abs; stamp ('move.l x,x',500000);
  bench_movem; stamp ('movem 48 bytes',50000);
  bench_stack; stamp ('pea/pop long',500000);
  bench_bsr; stamp ('bsr/rts',500000);
  x := readkey;
end.
