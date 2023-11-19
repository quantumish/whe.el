![](https://i.imgur.com/XtweqpI.png)

## control emacs with your steering wheel

This package implements Emacs support for the Thrustmaster T150 USB steering wheel. 

Not only can you control said steering wheel through Emacs, but `whe.el` both provides key emulation for all the buttons on the wheel to allow you to use them in keybinds. Furthermore, with the `wheel-def` macro you can have keybinds depend on the position of the wheel, thus making insane keybinds like "control-x-starboard" a reality.

```elisp
(wheel-def "C-x g"
   :starboard hydra-go-to-file/body
   :port magit-status
   :none vterm)
```
