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
$argadd inc/algo/frame.h
set stal=2
tabnew
tabnew
tabrewind
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
exe 'vert 2resize ' . ((&columns * 120 + 181) / 363)
exe 'vert 3resize ' . ((&columns * 121 + 181) / 363)
argglobal
balt inc/algo/frames.h
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
let s:l = 254 - ((52 * winheight(0) + 42) / 85)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 254
normal! 043|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/algo/frames.cpp") | buffer ~/Documents/master/pebbling-pdr/src/algo/frames.cpp | else | edit ~/Documents/master/pebbling-pdr/src/algo/frames.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
endif
balt ~/Documents/master/pebbling-pdr/src/algo/solver.cpp
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
let s:l = 159 - ((38 * winheight(0) + 42) / 85)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 159
normal! 050|
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp") | buffer ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp | else | edit ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
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
let s:l = 291 - ((59 * winheight(0) + 42) / 85)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 291
normal! 025|
lcd ~/Documents/master/pebbling-pdr
wincmd w
3wincmd w
exe 'vert 1resize ' . ((&columns * 120 + 181) / 363)
exe 'vert 2resize ' . ((&columns * 120 + 181) / 363)
exe 'vert 3resize ' . ((&columns * 121 + 181) / 363)
tabnext
edit ~/Documents/master/pebbling-pdr/inc/algo/frame.h
argglobal
balt ~/Documents/master/pebbling-pdr/src/algo/ipdr-peter.cpp
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
let s:l = 1 - ((0 * winheight(0) + 41) / 82)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 1
normal! 0
lcd ~/Documents/master/pebbling-pdr
tabnext
edit ~/Documents/master/pebbling-pdr/src/algo/ipdr-peter.cpp
argglobal
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
let s:l = 1 - ((0 * winheight(0) + 41) / 82)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 1
normal! 0
lcd ~/Documents/master/pebbling-pdr
tabnext 1
set stal=1
badd +43 ~/Documents/master/pebbling-pdr/inc/algo/frames.h
badd +33 ~/Documents/master/pebbling-pdr/inc/algo/frame.h
badd +175 ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp
badd +272 ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
badd +110 ~/Documents/master/pebbling-pdr/src/algo/frame.cpp
badd +1 ~/Documents/master/pebbling-pdr/src/algo/ipdr-peter.cpp
badd +291 ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
badd +73 ~/Documents/master/pebbling-pdr/src/algo/solver.cpp
badd +42 ~/Documents/master/pebbling-pdr/inc/algo/solver.h
if exists('s:wipebuf') && len(win_findbuf(s:wipebuf)) == 0 && getbufvar(s:wipebuf, '&buftype') isnot# 'terminal'
  silent exe 'bwipe ' . s:wipebuf
endif
unlet! s:wipebuf
set winheight=1 winwidth=20 shortmess=filnxtToOFAcI
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
