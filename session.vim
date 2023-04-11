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
$argadd src/algo/ipdr-pebbling.cpp
edit inc/algo/frames.h
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
wincmd _ | wincmd |
split
1wincmd k
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
exe 'vert 1resize ' . ((&columns * 101 + 158) / 317)
exe 'vert 2resize ' . ((&columns * 120 + 158) / 317)
exe '3resize ' . ((&lines * 39 + 41) / 82)
exe 'vert 3resize ' . ((&columns * 94 + 158) / 317)
exe '4resize ' . ((&lines * 39 + 41) / 82)
exe 'vert 4resize ' . ((&columns * 94 + 158) / 317)
argglobal
balt src/auxiliary/z3-ext.cpp
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
let s:l = 135 - ((67 * winheight(0) + 39) / 79)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 135
normal! 060|
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
let s:l = 231 - ((68 * winheight(0) + 39) / 79)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 231
normal! 06|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/algo/frame.cpp") | buffer ~/Documents/master/pebbling-pdr/src/algo/frame.cpp | else | edit ~/Documents/master/pebbling-pdr/src/algo/frame.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/algo/frame.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/algo/frame.h
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
let s:l = 41 - ((20 * winheight(0) + 19) / 39)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 41
normal! 019|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp") | buffer ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp | else | edit ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/algo/frame.h
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
let s:l = 321 - ((29 * winheight(0) + 19) / 39)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 321
normal! 073|
lcd ~/Documents/master/pebbling-pdr
wincmd w
2wincmd w
exe 'vert 1resize ' . ((&columns * 101 + 158) / 317)
exe 'vert 2resize ' . ((&columns * 120 + 158) / 317)
exe '3resize ' . ((&lines * 39 + 41) / 82)
exe 'vert 3resize ' . ((&columns * 94 + 158) / 317)
exe '4resize ' . ((&lines * 39 + 41) / 82)
exe 'vert 4resize ' . ((&columns * 94 + 158) / 317)
tabnext 1
badd +321 ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
badd +168 ~/Documents/master/pebbling-pdr/inc/algo/pdr.h
badd +218 ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
badd +58 ~/Documents/master/pebbling-pdr/inc/algo/frames.h
badd +42 ~/Documents/master/pebbling-pdr/inc/auxiliary/z3-ext.h
badd +281 ~/Documents/master/pebbling-pdr/src/auxiliary/z3-ext.cpp
badd +27 ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-context.cpp
badd +1 ~/Documents/master/pebbling-pdr/inc/algo/solver.h
badd +23 ~/Documents/master/pebbling-pdr/src/algo/solver.cpp
badd +178 ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-model.cpp
badd +141 ~/Documents/master/pebbling-pdr/inc/model/expr.h
badd +254 ~/Documents/master/pebbling-pdr/src/model/expr.cpp
badd +36 ~/Documents/master/pebbling-pdr/inc/algo/frame.h
badd +0 ~/Documents/master/pebbling-pdr/src/algo/frame.cpp
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
