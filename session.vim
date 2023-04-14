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
$argadd src/algo/frame.cpp
edit src/algo/frames.cpp
let s:save_splitbelow = &splitbelow
let s:save_splitright = &splitright
set splitbelow splitright
wincmd _ | wincmd |
vsplit
wincmd _ | wincmd |
vsplit
2wincmd h
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
exe 'vert 1resize ' . ((&columns * 120 + 181) / 363)
exe 'vert 2resize ' . ((&columns * 121 + 181) / 363)
exe 'vert 3resize ' . ((&columns * 120 + 181) / 363)
argglobal
balt src/algo/pdr.cpp
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
let s:l = 645 - ((75 * winheight(0) + 43) / 86)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 645
normal! 014|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/algo/frames.cpp") | buffer ~/Documents/master/pebbling-pdr/src/algo/frames.cpp | else | edit ~/Documents/master/pebbling-pdr/src/algo/frames.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/algo/frames.h
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
let s:l = 208 - ((40 * winheight(0) + 43) / 86)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 208
normal! 0
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/inc/algo/frames.h") | buffer ~/Documents/master/pebbling-pdr/inc/algo/frames.h | else | edit ~/Documents/master/pebbling-pdr/inc/algo/frames.h | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/inc/algo/frames.h
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
let s:l = 148 - ((50 * winheight(0) + 43) / 86)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 148
normal! 020|
lcd ~/Documents/master/pebbling-pdr
wincmd w
3wincmd w
exe 'vert 1resize ' . ((&columns * 120 + 181) / 363)
exe 'vert 2resize ' . ((&columns * 121 + 181) / 363)
exe 'vert 3resize ' . ((&columns * 120 + 181) / 363)
tabnext 1
badd +645 ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
badd +44 ~/Documents/master/pebbling-pdr/src/algo/frame.cpp
badd +134 ~/Documents/master/pebbling-pdr/inc/algo/frames.h
badd +287 ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
badd +279 ~/Documents/master/pebbling-pdr/inc/auxiliary/z3-ext.h
badd +160 ~/Documents/master/pebbling-pdr/src/algo/generalize.cpp
badd +10 ~/Documents/master/pebbling-pdr/src/auxiliary/z3-ext.cpp
badd +1 ~/Documents/master/pebbling-pdr/inc/algo/frame.h
badd +41 ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp
badd +40 ~/Documents/master/pebbling-pdr/inc/algo/solver.h
badd +44 ~/Documents/master/pebbling-pdr/src/algo/solver.cpp
badd +43 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-context.h
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
nohlsearch
doautoall SessionLoadPost
unlet SessionLoad
" vim: set ft=vim :
