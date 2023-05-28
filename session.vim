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
edit src/cli-parse.cpp
let s:save_splitbelow = &splitbelow
let s:save_splitright = &splitright
set splitbelow splitright
wincmd _ | wincmd |
vsplit
wincmd _ | wincmd |
vsplit
wincmd _ | wincmd |
vsplit
3wincmd h
wincmd w
wincmd w
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
exe 'vert 1resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 2resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 3resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 4resize ' . ((&columns * 89 + 181) / 362)
argglobal
balt inc/algo/pdr.h
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
let s:l = 274 - ((37 * winheight(0) + 41) / 83)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 274
normal! 067|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/algo/ipdr-peter.cpp") | buffer ~/Documents/master/pebbling-pdr/src/algo/ipdr-peter.cpp | else | edit ~/Documents/master/pebbling-pdr/src/algo/ipdr-peter.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/algo/ipdr-peter.cpp
endif
balt ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
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
let s:l = 95 - ((55 * winheight(0) + 41) / 83)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 95
normal! 08|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp") | buffer ~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp | else | edit ~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp
endif
balt ~/Documents/master/pebbling-pdr/src/testing/z3ipdr.cpp
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
let s:l = 194 - ((56 * winheight(0) + 41) / 83)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 194
normal! 05|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/testing/z3pebbling-experiments.cpp") | buffer ~/Documents/master/pebbling-pdr/src/testing/z3pebbling-experiments.cpp | else | edit ~/Documents/master/pebbling-pdr/src/testing/z3pebbling-experiments.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/testing/z3pebbling-experiments.cpp
endif
balt ~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp
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
let s:l = 15 - ((14 * winheight(0) + 41) / 83)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 15
normal! 0
lcd ~/Documents/master/pebbling-pdr
wincmd w
3wincmd w
exe 'vert 1resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 2resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 3resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 4resize ' . ((&columns * 89 + 181) / 362)
tabnext 1
badd +52 ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
badd +296 ~/Documents/master/pebbling-pdr/src/testing/stats.cpp
badd +47 ~/Documents/master/pebbling-pdr/inc/auxiliary/types-ext.h
badd +115 ~/Documents/master/pebbling-pdr/inc/algo/pdr.h
badd +38 ~/Documents/master/pebbling-pdr/inc/algo/vpdr.h
badd +317 ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
badd +39 ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-model.cpp
badd +195 ~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp
badd +31 ~/Documents/master/pebbling-pdr/inc/testing/z3pdr.h
badd +30 ~/Documents/master/pebbling-pdr/inc/tactic.h
badd +141 ~/Documents/master/pebbling-pdr/inc/cli-parse.h
badd +248 ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp
badd +174 ~/Documents/master/pebbling-pdr/inc/testing/stats.h
badd +264 ~/Documents/master/pebbling-pdr/src/cli-parse.cpp
badd +126 ~/Documents/master/pebbling-pdr/src/testing/z3ipdr.cpp
badd +43 ~/Documents/master/pebbling-pdr/inc/testing/z3-pebbling-model.h
badd +37 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-model.h
badd +34 ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-model.h
badd +39 ~/Documents/master/pebbling-pdr/src/testing/z3-pebbling-model.cpp
badd +25 ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-model.cpp
badd +106 ~/Documents/master/pebbling-pdr/inc/auxiliary/z3-ext.h
badd +270 ~/Documents/master/pebbling-pdr/src/auxiliary/z3-ext.cpp
badd +76 ~/Documents/master/pebbling-pdr/inc/model/peterson/peterson.h
badd +345 ~/Documents/master/pebbling-pdr/src/model/peterson/peterson.cpp
badd +49 ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp
badd +1 ~/Documents/master/pebbling-pdr/inc/testing/z3-pebbling-experiments.h
badd +35 ~/Documents/master/pebbling-pdr/src/testing/z3pebbling-experiments.cpp
badd +74 ~/Documents/master/pebbling-pdr/src/testing/pebbling-experiments.cpp
badd +95 ~/Documents/master/pebbling-pdr/src/algo/ipdr-peter.cpp
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
