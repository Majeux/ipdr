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
$argadd src/algo/frames.cpp
edit src/algo/frames.cpp
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
exe 'vert 1resize ' . ((&columns * 90 + 181) / 363)
exe 'vert 2resize ' . ((&columns * 90 + 181) / 363)
exe 'vert 3resize ' . ((&columns * 90 + 181) / 363)
exe 'vert 4resize ' . ((&columns * 90 + 181) / 363)
argglobal
balt src/algo/frame.cpp
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
let s:l = 603 - ((46 * winheight(0) + 42) / 84)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 603
normal! 0
wincmd w
argglobal
if bufexists("src/algo/ipdr-pebbling.cpp") | buffer src/algo/ipdr-pebbling.cpp | else | edit src/algo/ipdr-pebbling.cpp | endif
if &buftype ==# 'terminal'
  silent file src/algo/ipdr-pebbling.cpp
endif
balt src/algo/frames.cpp
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
let s:l = 326 - ((53 * winheight(0) + 42) / 84)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 326
normal! 05|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/algo/solver.cpp") | buffer ~/Documents/master/pebbling-pdr/src/algo/solver.cpp | else | edit ~/Documents/master/pebbling-pdr/src/algo/solver.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/algo/solver.cpp
endif
balt ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
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
let s:l = 88 - ((83 * winheight(0) + 42) / 84)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 88
normal! 0
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/algo/pdr.cpp") | buffer ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp | else | edit ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/algo/pdr.h
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
let s:l = 269 - ((66 * winheight(0) + 42) / 84)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 269
normal! 033|
wincmd w
exe 'vert 1resize ' . ((&columns * 90 + 181) / 363)
exe 'vert 2resize ' . ((&columns * 90 + 181) / 363)
exe 'vert 3resize ' . ((&columns * 90 + 181) / 363)
exe 'vert 4resize ' . ((&columns * 90 + 181) / 363)
tabnext 1
badd +598 ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
badd +0 ~/Documents/master/pebbling-pdr/src/algo/solver.cpp
badd +223 ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
badd +25 ~/Documents/master/pebbling-pdr/inc/algo/vpdr.h
badd +61 ~/Documents/master/pebbling-pdr/inc/algo/pdr.h
badd +269 ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp
badd +326 ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
badd +107 ~/Documents/master/pebbling-pdr/src/algo/frame.cpp
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
