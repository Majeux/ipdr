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
exe '1resize ' . ((&lines * 77 + 43) / 87)
exe 'vert 1resize ' . ((&columns * 119 + 181) / 363)
exe '2resize ' . ((&lines * 77 + 43) / 87)
exe 'vert 2resize ' . ((&columns * 125 + 181) / 363)
exe '3resize ' . ((&lines * 37 + 43) / 87)
exe 'vert 3resize ' . ((&columns * 116 + 181) / 363)
exe '4resize ' . ((&lines * 39 + 43) / 87)
exe 'vert 4resize ' . ((&columns * 116 + 181) / 363)
argglobal
balt inc/testing/z3pdr.h
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
let s:l = 49 - ((46 * winheight(0) + 38) / 77)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 49
normal! 05|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/testing/z3-pebbling-model.cpp") | buffer ~/Documents/master/pebbling-pdr/src/testing/z3-pebbling-model.cpp | else | edit ~/Documents/master/pebbling-pdr/src/testing/z3-pebbling-model.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/testing/z3-pebbling-model.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/testing/z3-pebbling-model.h
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
let s:l = 73 - ((36 * winheight(0) + 38) / 77)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 73
normal! 03|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp") | buffer ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp | else | edit ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/cli-parse.h
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
let s:l = 47 - ((0 * winheight(0) + 18) / 37)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 47
normal! 017|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/CMakeLists.txt") | buffer ~/Documents/master/pebbling-pdr/CMakeLists.txt | else | edit ~/Documents/master/pebbling-pdr/CMakeLists.txt | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/CMakeLists.txt
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
let s:l = 72 - ((0 * winheight(0) + 19) / 39)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 72
normal! 081|
lcd ~/Documents/master/pebbling-pdr
wincmd w
exe '1resize ' . ((&lines * 77 + 43) / 87)
exe 'vert 1resize ' . ((&columns * 119 + 181) / 363)
exe '2resize ' . ((&lines * 77 + 43) / 87)
exe 'vert 2resize ' . ((&columns * 125 + 181) / 363)
exe '3resize ' . ((&lines * 37 + 43) / 87)
exe 'vert 3resize ' . ((&columns * 116 + 181) / 363)
exe '4resize ' . ((&lines * 39 + 43) / 87)
exe 'vert 4resize ' . ((&columns * 116 + 181) / 363)
tabnext
edit ~/Documents/master/pebbling-pdr/src/algo/result.cpp
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
exe 'vert 1resize ' . ((&columns * 120 + 181) / 363)
exe '2resize ' . ((&lines * 39 + 43) / 87)
exe 'vert 2resize ' . ((&columns * 118 + 181) / 363)
exe '3resize ' . ((&lines * 43 + 43) / 87)
exe 'vert 3resize ' . ((&columns * 118 + 181) / 363)
exe 'vert 4resize ' . ((&columns * 123 + 181) / 363)
argglobal
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
let s:l = 186 - ((42 * winheight(0) + 41) / 83)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 186
normal! 052|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h") | buffer ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h | else | edit ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h
endif
balt ~/Documents/master/pebbling-pdr/inc/model/peterson/peterson-result.h
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
let s:l = 40 - ((30 * winheight(0) + 19) / 39)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 40
normal! 023|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp") | buffer ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp | else | edit ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp
endif
balt ~/Documents/master/pebbling-pdr/src/model/peterson/peterson-result.cpp
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
let s:l = 33 - ((21 * winheight(0) + 21) / 43)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 33
normal! 038|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/cli-parse.cpp") | buffer ~/Documents/master/pebbling-pdr/src/cli-parse.cpp | else | edit ~/Documents/master/pebbling-pdr/src/cli-parse.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/cli-parse.cpp
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
let s:l = 458 - ((41 * winheight(0) + 41) / 83)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 458
normal! 09|
lcd ~/Documents/master/pebbling-pdr
wincmd w
4wincmd w
exe 'vert 1resize ' . ((&columns * 120 + 181) / 363)
exe '2resize ' . ((&lines * 39 + 43) / 87)
exe 'vert 2resize ' . ((&columns * 118 + 181) / 363)
exe '3resize ' . ((&lines * 43 + 43) / 87)
exe 'vert 3resize ' . ((&columns * 118 + 181) / 363)
exe 'vert 4resize ' . ((&columns * 123 + 181) / 363)
tabnext 2
set stal=1
badd +65 ~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp
badd +58 ~/Documents/master/pebbling-pdr/src/testing/z3pebbling-experiments.cpp
badd +23 ~/Documents/master/pebbling-pdr/inc/testing/z3pdr.h
badd +79 ~/Documents/master/pebbling-pdr/src/testing/z3-pebbling-model.cpp
badd +50 ~/Documents/master/pebbling-pdr/inc/testing/z3-pebbling-model.h
badd +192 ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
badd +59 ~/Documents/master/pebbling-pdr/inc/cli-parse.h
badd +98 ~/Documents/master/pebbling-pdr/CMakeLists.txt
badd +17 ~/Documents/master/pebbling-pdr/inc/testing/z3-pebbling-experiments.h
badd +73 ~/Documents/master/pebbling-pdr/inc/testing/experiments.h
badd +149 ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp
badd +40 ~/Documents/master/pebbling-pdr/inc/testing/peterson-experiments.h
badd +70 ~/Documents/master/pebbling-pdr/src/testing/peterson-experiments.cpp
badd +16 ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-context.cpp
badd +23 ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-model.cpp
badd +7 ~/Documents/master/pebbling-pdr/src/testing/z3ipdr.cpp
badd +1 ~/Documents/master/pebbling-pdr/src/auxiliary/z3-ext.cpp
badd +61 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-model.h
badd +458 ~/Documents/master/pebbling-pdr/src/cli-parse.cpp
badd +26 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-context.h
badd +1 ~/Documents/master/pebbling-pdr/inc/testing/pebbling-experiments.h
badd +36 ~/Documents/master/pebbling-pdr/src/testing/pebbling-experiments.cpp
badd +104 ~/Documents/master/pebbling-pdr/inc/algo/result.h
badd +186 ~/Documents/master/pebbling-pdr/src/algo/result.cpp
badd +33 ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp
badd +29 ~/Documents/master/pebbling-pdr/src/model/peterson/peterson-result.cpp
badd +46 ~/Documents/master/pebbling-pdr/inc/model/peterson/peterson-result.h
badd +41 ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h
badd +70 ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
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
