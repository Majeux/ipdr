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
edit src/algo/generalize.cpp
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
exe 'vert 1resize ' . ((&columns * 120 + 181) / 362)
exe 'vert 2resize ' . ((&columns * 120 + 181) / 362)
exe 'vert 3resize ' . ((&columns * 120 + 181) / 362)
argglobal
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
let s:l = 281 - ((61 * winheight(0) + 40) / 80)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 281
normal! 012|
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
let s:l = 55 - ((54 * winheight(0) + 40) / 80)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 55
normal! 024|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/algo/solver.cpp") | buffer ~/Documents/master/pebbling-pdr/src/algo/solver.cpp | else | edit ~/Documents/master/pebbling-pdr/src/algo/solver.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/algo/solver.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/testing/stats.h
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
let s:l = 113 - ((49 * winheight(0) + 40) / 80)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 113
normal! 016|
lcd ~/Documents/master/pebbling-pdr
wincmd w
2wincmd w
exe 'vert 1resize ' . ((&columns * 120 + 181) / 362)
exe 'vert 2resize ' . ((&columns * 120 + 181) / 362)
exe 'vert 3resize ' . ((&columns * 120 + 181) / 362)
tabnext 1
badd +55 ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
badd +30 ~/Documents/master/pebbling-pdr/inc/algo/frames.h
badd +79 ~/Documents/master/pebbling-pdr/inc/testing/stats.h
badd +45 ~/Documents/master/pebbling-pdr/src/algo/solver.cpp
badd +47 ~/Documents/master/pebbling-pdr/inc/algo/solver.h
badd +22 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-context.h
badd +21 ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-context.cpp
badd +0 ~/Documents/master/pebbling-pdr/src/algo/generalize.cpp
badd +380 ~/Documents/master/pebbling-pdr/src/cli-parse.cpp
badd +137 ~/Documents/master/pebbling-pdr/inc/cli-parse.h
badd +2697 ~/vcpkg/installed/x64-linux/include/z3++.h
badd +216 ~/Documents/master/pebbling-pdr/inc/auxiliary/z3-ext.h
badd +157 ~/Documents/master/pebbling-pdr/src/testing/stats.cpp
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
