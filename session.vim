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
set stal=2
tabnew
tabrewind
edit src/pebbling-pdr.cpp
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
wincmd _ | wincmd |
split
1wincmd k
wincmd w
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
exe '1resize ' . ((&lines * 40 + 41) / 83)
exe 'vert 1resize ' . ((&columns * 90 + 181) / 362)
exe '2resize ' . ((&lines * 38 + 41) / 83)
exe 'vert 2resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 3resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 4resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 5resize ' . ((&columns * 89 + 181) / 362)
argglobal
balt src/testing/z3pdr.cpp
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
let s:l = 212 - ((18 * winheight(0) + 20) / 40)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 212
normal! 035|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/inc/testing/stats.h") | buffer ~/Documents/master/pebbling-pdr/inc/testing/stats.h | else | edit ~/Documents/master/pebbling-pdr/inc/testing/stats.h | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/inc/testing/stats.h
endif
balt ~/Documents/master/pebbling-pdr/inc/testing/logger.h
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
let s:l = 64 - ((37 * winheight(0) + 19) / 38)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 64
normal! 029|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h") | buffer ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h | else | edit ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h
endif
balt ~/Documents/master/pebbling-pdr/inc/algo/result.h
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
let s:l = 29 - ((28 * winheight(0) + 39) / 79)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 29
normal! 06|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/testing/z3ipdr.cpp") | buffer ~/Documents/master/pebbling-pdr/src/testing/z3ipdr.cpp | else | edit ~/Documents/master/pebbling-pdr/src/testing/z3ipdr.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/testing/z3ipdr.cpp
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
let s:l = 115 - ((57 * winheight(0) + 39) / 79)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 115
normal! 044|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/testing/z3-pebbling-model.cpp") | buffer ~/Documents/master/pebbling-pdr/src/testing/z3-pebbling-model.cpp | else | edit ~/Documents/master/pebbling-pdr/src/testing/z3-pebbling-model.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/testing/z3-pebbling-model.cpp
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
let s:l = 83 - ((47 * winheight(0) + 39) / 79)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 83
normal! 03|
lcd ~/Documents/master/pebbling-pdr
wincmd w
exe '1resize ' . ((&lines * 40 + 41) / 83)
exe 'vert 1resize ' . ((&columns * 90 + 181) / 362)
exe '2resize ' . ((&lines * 38 + 41) / 83)
exe 'vert 2resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 3resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 4resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 5resize ' . ((&columns * 89 + 181) / 362)
tabnext
edit ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
let s:save_splitbelow = &splitbelow
let s:save_splitright = &splitright
set splitbelow splitright
wincmd _ | wincmd |
vsplit
wincmd _ | wincmd |
vsplit
2wincmd h
wincmd w
wincmd _ | wincmd |
split
1wincmd k
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
exe '2resize ' . ((&lines * 39 + 41) / 83)
exe 'vert 2resize ' . ((&columns * 120 + 181) / 362)
exe '3resize ' . ((&lines * 39 + 41) / 83)
exe 'vert 3resize ' . ((&columns * 120 + 181) / 362)
exe 'vert 4resize ' . ((&columns * 120 + 181) / 362)
argglobal
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
let s:l = 64 - ((39 * winheight(0) + 39) / 79)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 64
normal! 023|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h") | buffer ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h | else | edit ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h
endif
balt ~/Documents/master/pebbling-pdr/src/testing/z3-pebbling-model.cpp
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
let s:l = 55 - ((31 * winheight(0) + 19) / 39)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 55
normal! 078|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/algo/result.cpp") | buffer ~/Documents/master/pebbling-pdr/src/algo/result.cpp | else | edit ~/Documents/master/pebbling-pdr/src/algo/result.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/algo/result.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/algo/result.h
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
let s:l = 375 - ((19 * winheight(0) + 19) / 39)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 375
normal! 036|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp") | buffer ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp | else | edit ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h
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
let s:l = 93 - ((0 * winheight(0) + 39) / 79)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 93
normal! 025|
lcd ~/Documents/master/pebbling-pdr
wincmd w
2wincmd w
exe 'vert 1resize ' . ((&columns * 120 + 181) / 362)
exe '2resize ' . ((&lines * 39 + 41) / 83)
exe 'vert 2resize ' . ((&columns * 120 + 181) / 362)
exe '3resize ' . ((&lines * 39 + 41) / 83)
exe 'vert 3resize ' . ((&columns * 120 + 181) / 362)
exe 'vert 4resize ' . ((&columns * 120 + 181) / 362)
tabnext 2
set stal=1
badd +105 ~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp
badd +110 ~/Documents/master/pebbling-pdr/src/testing/z3-pebbling-model.cpp
badd +375 ~/Documents/master/pebbling-pdr/src/algo/result.cpp
badd +131 ~/Documents/master/pebbling-pdr/inc/algo/result.h
badd +84 ~/Documents/master/pebbling-pdr/src/testing/z3ipdr.cpp
badd +55 ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h
badd +64 ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
badd +83 ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
badd +12 ~/Documents/master/pebbling-pdr/src/testing/logger.cpp
badd +156 ~/Documents/master/pebbling-pdr/inc/testing/logger.h
badd +0 ~/Documents/master/pebbling-pdr/inc/testing/stats.h
badd +18 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-context.h
badd +8 ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-context.cpp
badd +14 ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-model.cpp
badd +39 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-model.h
badd +36 ~/Documents/master/pebbling-pdr/inc/algo/frames.h
badd +58 ~/Documents/master/pebbling-pdr/inc/algo/pdr.h
badd +38 ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
badd +103 ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp
badd +33 ~/Documents/master/pebbling-pdr/src/algo/pdr-alt.cpp
badd +41 ~/Documents/master/pebbling-pdr/src/algo/generalize.cpp
badd +63 ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp
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
