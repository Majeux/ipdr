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
$argadd src/model/peterson/peterson-result.cpp
edit src/algo/ipdr-peter.cpp
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
exe 'vert 3resize ' . ((&columns * 88 + 181) / 362)
exe 'vert 4resize ' . ((&columns * 91 + 181) / 362)
argglobal
balt src/algo/result.cpp
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
let s:l = 66 - ((15 * winheight(0) + 40) / 80)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 66
normal! 027|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/algo/frames.cpp") | buffer ~/Documents/master/pebbling-pdr/src/algo/frames.cpp | else | edit ~/Documents/master/pebbling-pdr/src/algo/frames.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/model/expr.h
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
let s:l = 198 - ((24 * winheight(0) + 40) / 80)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 198
normal! 016|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/model/peterson/peterson.cpp") | buffer ~/Documents/master/pebbling-pdr/src/model/peterson/peterson.cpp | else | edit ~/Documents/master/pebbling-pdr/src/model/peterson/peterson.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/model/peterson/peterson.cpp
endif
balt ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
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
let s:l = 555 - ((44 * winheight(0) + 40) / 80)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 555
normal! 080|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/auxiliary/z3-ext.cpp") | buffer ~/Documents/master/pebbling-pdr/src/auxiliary/z3-ext.cpp | else | edit ~/Documents/master/pebbling-pdr/src/auxiliary/z3-ext.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/auxiliary/z3-ext.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/auxiliary/z3-ext.h
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
let s:l = 84 - ((47 * winheight(0) + 40) / 80)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 84
normal! 0
lcd ~/Documents/master/pebbling-pdr
wincmd w
2wincmd w
exe 'vert 1resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 2resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 3resize ' . ((&columns * 88 + 181) / 362)
exe 'vert 4resize ' . ((&columns * 91 + 181) / 362)
tabnext 1
badd +137 ~/Documents/master/pebbling-pdr/inc/algo/frames.h
badd +151 ~/Documents/master/pebbling-pdr/src/model/peterson/peterson-result.cpp
badd +620 ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
badd +66 ~/Documents/master/pebbling-pdr/src/algo/ipdr-peter.cpp
badd +459 ~/Documents/master/pebbling-pdr/src/model/peterson/peterson.cpp
badd +110 ~/Documents/master/pebbling-pdr/inc/model/peterson/peterson.h
badd +67 ~/Documents/master/pebbling-pdr/src/auxiliary/z3-ext.cpp
badd +512 ~/Documents/master/pebbling-pdr/src/cli-parse.cpp
badd +14 ~/Documents/master/pebbling-pdr/.clang-format
badd +220 ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
badd +55 ~/Documents/master/pebbling-pdr/src/testing/peterson-experiments.cpp
badd +1 ~/Documents/master/pebbling-pdr/src/model/expr.cpp
badd +309 ~/Documents/master/pebbling-pdr/inc/model/expr.h
badd +1 ~/Documents/master/pebbling-pdr/src/algo/solver.cpp
badd +170 ~/Documents/master/pebbling-pdr/inc/testing/stats.h
badd +55 ~/Documents/master/pebbling-pdr/inc/auxiliary/z3-ext.h
badd +224 ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp
badd +324 ~/Documents/master/pebbling-pdr/src/testing/stats.cpp
badd +52 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-model.h
badd +29 ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-model.h
badd +196 ~/Documents/master/pebbling-pdr/inc/cli-parse.h
badd +96 ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp
badd +102 ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
badd +165 ~/Documents/master/pebbling-pdr/inc/algo/pdr.h
badd +47 ~/Documents/master/pebbling-pdr/inc/model/peterson/peterson-result.h
badd +131 ~/Documents/master/pebbling-pdr/inc/algo/bounded.h
badd +22 ~/Documents/master/pebbling-pdr/src/testing/z3ipdr.cpp
badd +77 ~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp
badd +31 ~/Documents/master/pebbling-pdr/rls_experiments-peter.sh
badd +1 ~/Documents/master/pebbling-pdr/inc/testing/peterson-experiments.h
badd +392 ~/Documents/master/pebbling-pdr/src/algo/result.cpp
badd +174 ~/Documents/master/pebbling-pdr/inc/algo/result.h
badd +21 ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h
badd +219 ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp
badd +36 ~/Documents/master/pebbling-pdr/inc/model/dag.h
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
