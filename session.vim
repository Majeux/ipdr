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
$argadd src/testing/z3pdr.cpp
edit src/testing/z3pdr.cpp
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
exe 'vert 1resize ' . ((&columns * 78 + 181) / 362)
exe 'vert 2resize ' . ((&columns * 86 + 181) / 362)
exe 'vert 3resize ' . ((&columns * 196 + 181) / 362)
argglobal
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
let s:l = 81 - ((29 * winheight(0) + 40) / 80)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 81
normal! 026|
wincmd w
argglobal
if bufexists("src/testing/z3-pebbling-model.cpp") | buffer src/testing/z3-pebbling-model.cpp | else | edit src/testing/z3-pebbling-model.cpp | endif
if &buftype ==# 'terminal'
  silent file src/testing/z3-pebbling-model.cpp
endif
balt inc/testing/z3-pebbling-model.h
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
let s:l = 157 - ((47 * winheight(0) + 40) / 80)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 157
normal! 06|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp") | buffer ~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp | else | edit ~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp
endif
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
let s:l = 54 - ((53 * winheight(0) + 40) / 80)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 54
normal! 018|
wincmd w
3wincmd w
exe 'vert 1resize ' . ((&columns * 78 + 181) / 362)
exe 'vert 2resize ' . ((&columns * 86 + 181) / 362)
exe 'vert 3resize ' . ((&columns * 196 + 181) / 362)
tabnext 1
badd +1 ~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp
badd +156 ~/Documents/master/pebbling-pdr/src/testing/z3-pebbling-model.cpp
badd +1 ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-model.cpp
badd +50 ~/Documents/master/pebbling-pdr/inc/testing/z3-pebbling-model.h
badd +61 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-model.h
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
