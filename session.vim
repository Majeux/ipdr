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
$argadd src/algo/pdr.cpp
edit src/algo/pdr.cpp
let s:save_splitbelow = &splitbelow
let s:save_splitright = &splitright
set splitbelow splitright
wincmd _ | wincmd |
vsplit
wincmd _ | wincmd |
vsplit
2wincmd h
wincmd _ | wincmd |
split
1wincmd k
wincmd w
wincmd w
wincmd _ | wincmd |
split
1wincmd k
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
exe '1resize ' . ((&lines * 41 + 43) / 87)
exe 'vert 1resize ' . ((&columns * 120 + 181) / 363)
exe '2resize ' . ((&lines * 42 + 43) / 87)
exe 'vert 2resize ' . ((&columns * 120 + 181) / 363)
exe '3resize ' . ((&lines * 42 + 43) / 87)
exe 'vert 3resize ' . ((&columns * 119 + 181) / 363)
exe '4resize ' . ((&lines * 41 + 43) / 87)
exe 'vert 4resize ' . ((&columns * 119 + 181) / 363)
exe '5resize ' . ((&lines * 63 + 43) / 87)
exe 'vert 5resize ' . ((&columns * 122 + 181) / 363)
exe '6resize ' . ((&lines * 20 + 43) / 87)
exe 'vert 6resize ' . ((&columns * 122 + 181) / 363)
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
let s:l = 36 - ((14 * winheight(0) + 20) / 41)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 36
normal! 011|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/inc/algo/pdr.h") | buffer ~/Documents/master/pebbling-pdr/inc/algo/pdr.h | else | edit ~/Documents/master/pebbling-pdr/inc/algo/pdr.h | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/inc/algo/pdr.h
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
let s:l = 127 - ((0 * winheight(0) + 21) / 42)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 127
normal! 0
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/inc/algo/vpdr.h") | buffer ~/Documents/master/pebbling-pdr/inc/algo/vpdr.h | else | edit ~/Documents/master/pebbling-pdr/inc/algo/vpdr.h | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/inc/algo/vpdr.h
endif
balt ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-context.cpp
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
let s:l = 1 - ((0 * winheight(0) + 21) / 42)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 1
normal! 0
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp") | buffer ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp | else | edit ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
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
let s:l = 31 - ((21 * winheight(0) + 20) / 41)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 31
normal! 033|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp") | buffer ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp | else | edit ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
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
let s:l = 240 - ((35 * winheight(0) + 31) / 63)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 240
normal! 08|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/small_rls.txt") | buffer ~/Documents/master/pebbling-pdr/small_rls.txt | else | edit ~/Documents/master/pebbling-pdr/small_rls.txt | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/small_rls.txt
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
let s:l = 1 - ((0 * winheight(0) + 10) / 20)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 1
normal! 0
lcd ~/Documents/master/pebbling-pdr
wincmd w
2wincmd w
exe '1resize ' . ((&lines * 41 + 43) / 87)
exe 'vert 1resize ' . ((&columns * 120 + 181) / 363)
exe '2resize ' . ((&lines * 42 + 43) / 87)
exe 'vert 2resize ' . ((&columns * 120 + 181) / 363)
exe '3resize ' . ((&lines * 42 + 43) / 87)
exe 'vert 3resize ' . ((&columns * 119 + 181) / 363)
exe '4resize ' . ((&lines * 41 + 43) / 87)
exe 'vert 4resize ' . ((&columns * 119 + 181) / 363)
exe '5resize ' . ((&lines * 63 + 43) / 87)
exe 'vert 5resize ' . ((&columns * 122 + 181) / 363)
exe '6resize ' . ((&lines * 20 + 43) / 87)
exe 'vert 6resize ' . ((&columns * 122 + 181) / 363)
tabnext 1
badd +36 ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp
badd +32 ~/Documents/master/pebbling-pdr/inc/algo/frames.h
badd +5 ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-context.cpp
badd +26 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-context.h
badd +14 ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
badd +136 ~/Documents/master/pebbling-pdr/inc/algo/pdr.h
badd +303 ~/Documents/master/pebbling-pdr/src/cli-parse.cpp
badd +1 ~/Documents/master/pebbling-pdr/src/testing/z3ipdr.cpp
badd +1 ~/Documents/master/pebbling-pdr/small_rls.txt
badd +114 ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
badd +237 ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
badd +112 ~/Documents/master/pebbling-pdr/src/algo/generalize.cpp
badd +178 ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-model.cpp
badd +13 ~/Documents/master/pebbling-pdr/inc/algo/vpdr.h
badd +46 ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-model.h
badd +8 ~/Documents/master/pebbling-pdr/src/algo/ipdr-peter.cpp
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
