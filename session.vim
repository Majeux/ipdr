let SessionLoad = 1
let s:so_save = &g:so | let s:siso_save = &g:siso | setg so=0 siso=0 | setl so=-1 siso=-1
let v:this_session=expand("<sfile>:p")
silent only
silent tabonly
cd ~/Documents/master/pebbling-pdr
if expand('%') == '' && !&modified && line('$') <= 1 && getline(1) == ''
  let s:wipebuf = bufnr('%')
endif
set shortmess=aoO
argglobal
%argdel
$argadd src/testing/stats.cpp
edit src/model/peterson/peterson.cpp
let s:save_splitbelow = &splitbelow
let s:save_splitright = &splitright
set splitbelow splitright
wincmd _ | wincmd |
vsplit
1wincmd h
wincmd w
let &splitbelow = s:save_splitbelow
let &splitright = s:save_splitright
wincmd t
let s:save_winminheight = &winminheight
let s:save_winminwidth = &winminwidth
set winminheight=0
set winheight=1
set winminwidth=0
set winwidth=1
exe 'vert 1resize ' . ((&columns * 112 + 112) / 225)
exe 'vert 2resize ' . ((&columns * 112 + 112) / 225)
argglobal
balt inc/model/peterson/peterson.h
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let &fdl = &fdl
let s:l = 1134 - ((34 * winheight(0) + 26) / 52)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 1134
normal! 05|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/model/peterson/peterson.cpp") | buffer ~/Documents/master/pebbling-pdr/src/model/peterson/peterson.cpp | else | edit ~/Documents/master/pebbling-pdr/src/model/peterson/peterson.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/model/peterson/peterson.cpp
endif
balt ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-model.cpp
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let &fdl = &fdl
let s:l = 662 - ((0 * winheight(0) + 26) / 52)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 662
normal! 033|
lcd ~/Documents/master/pebbling-pdr
wincmd w
exe 'vert 1resize ' . ((&columns * 112 + 112) / 225)
exe 'vert 2resize ' . ((&columns * 112 + 112) / 225)
tabnext 1
badd +2 ~/Documents/master/pebbling-pdr/inc/model/peterson/peterson.h
badd +0 ~/Documents/master/pebbling-pdr/src/testing/stats.cpp
badd +137 ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-model.cpp
badd +1140 ~/Documents/master/pebbling-pdr/src/model/peterson/peterson.cpp
badd +357 ~/Documents/master/pebbling-pdr/src/model/expr.cpp
badd +159 ~/Documents/master/pebbling-pdr/inc/algo/pdr.h
badd +0 ~/Documents/master/pebbling-pdr/inc/testing/z3-pebbling-model.h
badd +65 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-model.h
badd +102 ~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp
badd +33 ~/Documents/master/pebbling-pdr/inc/testing/z3pdr.h
badd +41 ~/Documents/master/pebbling-pdr/inc/algo/vpdr.h
badd +1 ~/Documents/master/pebbling-pdr/src/algo/pdr-logging.cpp
badd +113 ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp
badd +306 ~/Documents/master/pebbling-pdr/src/cli-parse.cpp
badd +115 ~/Documents/master/pebbling-pdr/src/algo/ipdr-peter.cpp
badd +317 ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
badd +126 ~/Documents/master/pebbling-pdr/src/testing/z3ipdr.cpp
badd +46 ~/Documents/master/pebbling-pdr/src/testing/z3pebbling-experiments.cpp
badd +52 ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
badd +47 ~/Documents/master/pebbling-pdr/inc/auxiliary/types-ext.h
badd +39 ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-model.cpp
badd +30 ~/Documents/master/pebbling-pdr/inc/tactic.h
badd +141 ~/Documents/master/pebbling-pdr/inc/cli-parse.h
badd +248 ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp
badd +174 ~/Documents/master/pebbling-pdr/inc/testing/stats.h
badd +34 ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-model.h
badd +39 ~/Documents/master/pebbling-pdr/src/testing/z3-pebbling-model.cpp
badd +105 ~/Documents/master/pebbling-pdr/inc/auxiliary/z3-ext.h
badd +280 ~/Documents/master/pebbling-pdr/src/auxiliary/z3-ext.cpp
badd +1 ~/Documents/master/pebbling-pdr/inc/testing/z3-pebbling-experiments.h
badd +74 ~/Documents/master/pebbling-pdr/src/testing/pebbling-experiments.cpp
if exists('s:wipebuf') && len(win_findbuf(s:wipebuf)) == 0 && getbufvar(s:wipebuf, '&buftype') isnot# 'terminal'
  silent exe 'bwipe ' . s:wipebuf
endif
unlet! s:wipebuf
set winheight=1 winwidth=20 shortmess=filnxtToOFAcI
let &winminheight = s:save_winminheight
let &winminwidth = s:save_winminwidth
let s:sx = expand("<sfile>:p:r")."x.vim"
if filereadable(s:sx)
  exe "source " . fnameescape(s:sx)
endif
let &g:so = s:so_save | let &g:siso = s:siso_save
set hlsearch
doautoall SessionLoadPost
unlet SessionLoad
" vim: set ft=vim :
