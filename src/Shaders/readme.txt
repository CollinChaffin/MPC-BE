Some shaders are taken from "Video pixel shader pack v1.4" and "MPC-HC tester builds" by Jan-Willem Krans.

One-pass resizers is not used in the player, but can be used for information and testing. Do not delete these files.

"resizer_catmull4.hlsl", "resizer_lanczos3_x.hlsl", "resizer_lanczos3_y.hlsl" can not be compiled with the profile ps_2_0.

"final_pass.hlsl" is compiled only for verification. The player is added to the source code of the shader, which is compiled at runtime.
