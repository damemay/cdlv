syn match       type            "!bg"
syn match       type            "!anim"
syn match       type            "!anim_once"
syn match       type            "!anim_wait"
syn match       type            "!anim_text"
syn match       type            "!script"       contained
syn match       scene           "!scene"
syn match       img             "@.*\|\s\+\d*\s\+"          contained
syn match       size            "^\x*\s\x*\s\x*\s\S*"
syn region      res             matchgroup=scene end="!scene"he=e-6 matchgroup=type start="!anim_once$\|!anim_wait$" skipempty skipnl skipwhite
syn region      res2            matchgroup=type end="!script"he=e-7 matchgroup=type start="!anim$\|!bg\|!anim_text$" skipempty skipnl skipwhite contains=type
syn region      script          matchgroup=scene end="!scene" matchgroup=type start="!script" contains=scene,type,img

hi link type    Type
hi link img     Identifier
hi link size    PreProc
hi link scene   Statement
hi link res     Comment
hi link res2    Comment
hi link script  Constant
