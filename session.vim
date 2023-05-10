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
edit inc/algo/frames.h
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
exe 'vert 1resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 2resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 3resize ' . ((&columns * 90 + 181) / 362)
exe '4resize ' . ((&lines * 39 + 41) / 83)
exe 'vert 4resize ' . ((&columns * 89 + 181) / 362)
exe '5resize ' . ((&lines * 40 + 41) / 83)
exe 'vert 5resize ' . ((&columns * 89 + 181) / 362)
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
let s:l = 97 - ((0 * winheight(0) + 40) / 80)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 97
normal! 0
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/algo/frames.cpp") | buffer ~/Documents/master/pebbling-pdr/src/algo/frames.cpp | else | edit ~/Documents/master/pebbling-pdr/src/algo/frames.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
endif
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
let s:l = 605 - ((30 * winheight(0) + 40) / 80)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 605
normal! 016|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/model/peterson/peterson.cpp") | buffer ~/Documents/master/pebbling-pdr/src/model/peterson/peterson.cpp | else | edit ~/Documents/master/pebbling-pdr/src/model/peterson/peterson.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/model/peterson/peterson.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/model/peterson/peterson.h
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
let s:l = 434 - ((47 * winheight(0) + 40) / 80)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 434
normal! 011|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/auxiliary/z3-ext.cpp") | buffer ~/Documents/master/pebbling-pdr/src/auxiliary/z3-ext.cpp | else | edit ~/Documents/master/pebbling-pdr/src/auxiliary/z3-ext.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/auxiliary/z3-ext.cpp
endif
balt ~/Documents/master/pebbling-pdr/src/cli-parse.cpp
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
let s:l = 109 - ((19 * winheight(0) + 19) / 39)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 109
normal! 05|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/.clang-format") | buffer ~/Documents/master/pebbling-pdr/.clang-format | else | edit ~/Documents/master/pebbling-pdr/.clang-format | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/.clang-format
endif
balt ~/Documents/master/pebbling-pdr/src/auxiliary/z3-ext.cpp
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
let s:l = 27 - ((26 * winheight(0) + 20) / 40)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 27
normal! 017|
lcd ~/Documents/master/pebbling-pdr
wincmd w
4wincmd w
exe 'vert 1resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 2resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 3resize ' . ((&columns * 90 + 181) / 362)
exe '4resize ' . ((&lines * 39 + 41) / 83)
exe 'vert 4resize ' . ((&columns * 89 + 181) / 362)
exe '5resize ' . ((&lines * 40 + 41) / 83)
exe 'vert 5resize ' . ((&columns * 89 + 181) / 362)
tabnext 1
badd +1 ~/Documents/master/pebbling-pdr/inc/algo/frames.h
badd +1 ~/Documents/master/pebbling-pdr/src/model/peterson/peterson-result.cpp
badd +228 ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
badd +1 ~/Documents/master/pebbling-pdr/src/algo/ipdr-peter.cpp
badd +329 ~/Documents/master/pebbling-pdr/src/model/peterson/peterson.cpp
badd +48 ~/Documents/master/pebbling-pdr/inc/model/peterson/peterson.h
badd +0 ~/Documents/master/pebbling-pdr/src/auxiliary/z3-ext.cpp
badd +415 ~/Documents/master/pebbling-pdr/src/cli-parse.cpp
badd +27 ~/Documents/master/pebbling-pdr/.clang-format
badd +173 ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
badd +19 ~/Documents/master/pebbling-pdr/src/testing/peterson-experiments.cpp
badd +1 ~/Documents/master/pebbling-pdr/src/model/expr.cpp
badd +218 ~/Documents/master/pebbling-pdr/inc/model/expr.h
badd +1 ~/Documents/master/pebbling-pdr/src/algo/solver.cpp
badd +132 ~/Documents/master/pebbling-pdr/inc/testing/stats.h
badd +88 ~/Documents/master/pebbling-pdr/inc/auxiliary/z3-ext.h
badd +209 ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp
badd +167 ~/Documents/master/pebbling-pdr/src/testing/stats.cpp
badd +52 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-model.h
badd +29 ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-model.h
badd +195 ~/Documents/master/pebbling-pdr/inc/cli-parse.h
badd +144 ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp
badd +109 ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
badd +181 ~/Documents/master/pebbling-pdr/inc/algo/pdr.h
badd +29 ~/Documents/master/pebbling-pdr/inc/model/peterson/peterson-result.h
badd +131 ~/Documents/master/pebbling-pdr/inc/algo/bounded.h
badd +22 ~/Documents/master/pebbling-pdr/src/testing/z3ipdr.cpp
badd +77 ~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp
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
