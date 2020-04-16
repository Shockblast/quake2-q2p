

Quake2 modified and built by QuDos at http://qudos.quakedev.com.
Based on vanilla 3.21 sources.

##################################################################################
# WARNING:                                                                       #
# Mods built with "-fomit-frame-pointer" and/or "-ffast-math" flags and some gcc #
# versions can be a cause of crashes.                                            #
# Solution: rebuild the mod with these flags out or build a debug game binary.   #
##################################################################################


Installation --

  -Required data to run q2p is packaged with the sources under data folder:
   http://qudos.quakedev.com/linux/quake2/engines/Q2P/Q2P-version-date.tar.bz2 
  
  -Suggested to remove/backup old q2p.cfg from baseq2 and/or mods prior to run a new version or release.
  
  -Run q2p, note that you must to set $(HOME)/bin available by executable search path.
   Create a bin folder in your home, make a symlink pointing to q2p.run
   
     mkdir ~/bin && cd ~/bin && ln -s ~/quake2/q2p.run q2p
     
     and finally add to .bash_profile hidden file.
     
     if [ -d ~/bin ] ; then
      PATH=~/bin:"${PATH}"
     fi
    
  -*nix users if get compiled this thing, by doing make install will install the
   full distribution under ~/quake2, only required pak0.pak from your quake2 cdrom
   and also the full ctf patch from idsoftware or www.quakedev.com/files
   
  -win32 users must to copy the required q2p.q2z from data folder in this file,
   also pak0.pak and full patch.
  
  Main changes to the original quake2 distribution:
  
   -OpenGL only, all related with software was removed.
   -Renamed quake2 and video refs to be q2p/q2p.exe and vid_gl.so/vid_gl.dll.
   -q2p-nopart and vid_*-nopart are binaries with no Psychospaz particle system.
    Just rename them to use.
   -Load by default vid_gl.so/vid_gl.dll.
   -Alsa, OSS and SDL sound libraries available for unix.
   -Writting to q2p.cfg.
   -Compressed files support, will load zip files with .q2z extension.
   -Ogg Vorbis support, see the Ogg readme file for better explanation.
   -Retexture, Jpeg, Png support.
   -Jpeg screenshots, keeping gamma.
   -Detailed textures.
   -Global fog.
   -Underwater fog.
   -Water reflection.
   -Water shader reflection/distortion.
   -Under water caustics.
   -Under water transparency.
   -Under water movement.
   -Water waves.
   -Sniper Scope.
   -Decals on shots.
   -Motion Blur.
   -Some extra GL extensions.
   -Lightmap Saturation.
   -FPS counter.
   -Date, clock, timestamped messages, ingame map time, hightlight name players.
   -Ping, net rate, chat hud.
   -Enhanced box for netgraph.
   -Transparent and resizable console.
   -Resizable pics on the fly.
   -Sky distance.
   -Enhanced complete commands by pressing the TAB key.
   -Console mouse scroll support, clock and date, customizable transparency and size.
   -By pressing Alt+Enter switch fullscreen/windowed mode or viceversa.
   -Removed DirectSound in flavour of Waveform Audio, courtesy of Ben Lane.
   -Quake2Max 0.45 particle system, not finished but stable, in order to get the old 
    particle system uncomment the line #define PARTICLESYSTEM at the top of game/q_shared.h
   -Scalable fonts.
   -Scalable hud pics.
   -Mouse in menus.
   -Image levels loading.
   -XMMS, Audacious, Winamp players control. Add them to menus TODO
   -Big hack to load old mods, nice job from Skuller, big thanks.
   -Most of new options were added to menus.
   -Type cvarlist at the console prompt to get the complete variable list,
    some variables can do nothing because were not finished or disabled for now.
   
   
   --See the complete list of new available commands at the bottom of this file, sure i forgot 
     something, no problem, this readme is alpha yet =)
   
   --Important note about distortion water reflection shader.
     Note that you card must support this feature, not all cards does support it.
     An easy way to see if is supported is typing gl_strings and searching for 'GL_ARB_fragment_program', 
     if it's supported the console will show '...using GL_ARB_fragment_program'
     The console output will advice you if is supported, available or enabled.
     Supported? ok, now you must to set to 1 the variable gl_water_reflection_fragment_program, 
     then a vid_restart and now you can set gl_water_reflection_shader to 1.
     
   --About Hardware gamma and video modes on windows os, the brightness works in game by setting vid_gamma in
     console or by setting the slider in video menus, but originally if you press the key ESC it will make a
     vid restart, this was changed, now there is no video restart so in order to get the video mode selected
     you must to press the key ENTER.
     
All this work is based in tutorials from Quake Standars Group, and taking/modifying code from other engines 
    like, paintball2, kmquake2, quake2max, aq2, q2xp, ..., the water code distortion is a very great reworked
    Dukey's job, Ogg Vorbis Ufo AI code rewrote by Ale, so the credits to all them...
 
    Knightmare,
    Dukey,
    Jitspoe,
    Psychospaz,
    Matt Ownby,
    Maniac,
    MH,
    Q2Icculus,
    Quake2Forge,
    Barnes,
    and many others......., thanks
   
 //______________________________________________________________________________________________________________\\
 
 
 New Variables
  
  ufff = test for your self
  
  cl_drawfps; 0/1 (0)
  cl_drawdate; 0/1 (0)
  cl_drawclock; 0/1/2 12/24 Hr format (0)
  cl_drawtimestamps; 0/1 (1)
  cl_drawping; 0/1 (0)
  cl_drawrate; 0/1 (0)
  cl_drawchathud; 0/1 (1)
  cl_drawmaptime; 0/1 (0)
  cl_highlight; 0/1/2 (1)
  cl_draw_x; 0-0.99 (0) X position for the above variables
  cl_draw_y; 0-0.99 (0.8) Y position for the above variables
  netgraph*; 0/1/2/3 (1) , press the key tab to see more options for netgraph like size, position, scale, ...
  crosshair*; Various options, key TAB plz
  cl_hud*; color and alpha channel options for pics (numbers) on the hud, key TAB plz
  con_trans; 0-1 (0.5) Console transparency
  con_height; 0-1 (0.5) Console size
  cl_underwater_trans; 0/1 (0)
  cl_underwater_movement; 0/1 (1)
  gl_overbrightbits; 0/1/2 (2)
  gl_ext_mtexcombine; 0/1 (0)
  gl_ext_texture_compression; 0/1 (0)
  gl_motionblur; 0/1/2 (0) (where 1 is only underwater)
  gl_motionblur_intensity; 0-1 (0.6)
  gl_skydistance; ufff (3500)
  gl_water_waves; 0-10 (0) 
  gl_water_caustics; 0/1 (0)
  gl_water_caustics_image; defaults to /textures/water/caustics.pcx (caustics-caustics8)
                           (note also that you must to type the full name of the pic, with the extension,
                            required vid_restart)
  gl_detailtextures; 0-x.x (0) (a value > 10 is recommended, i.e 15.0)
  gl_water_reflection; 0-1 (0) (a value 0.3/0.6 is recomended)
  gl_water_reflection_debug; 0/1 (0)
  gl_water_reflection_max; ufff (2)
  gl_water_reflection_shader; 0/1 (0)
  gl_water_reflection_shader_image; defaults to /textures/water/distortion.pcx (distortion-distortion11)
                                    (note also that you must to type the full name of the pic, 
                                     with the extension, some of them can get better or lower fps, 
                                     required vid_restart)
  gl_water_reflection_fragment_program; 0/1 (0) See the above information about this feature
                                                before running it.
  gl_reflection_water_surf; 0/1 (1) 
  gl_fogenable; 0/1 (0)
  gl_fogstart; ufff (50)
  gl_fogend; ufff (1500)
  gl_fogdensity; ufff (0.001)
  gl_fogred; 0-1 (0.4)
  gl_foggreen; 0-1 (0.4)
  gl_fogblue; 0-1 (0.4)
  gl_fogunderwater; 0/1 (0)
  gl_screenshot_jpeg; 0/1 (1)
  gl_screenshot_jpeg_quality; 0-100 (95)
  gl_decals; 0/1 (1)
  gl_decals_time; ufff (30)
  gl_lightmap_saturation; 0-1 (1) Value 0 will turn the game black and white
  gl_lightmap_texture_saturation; 0-1 (1) Value 0 will turn the game black and white
  ogg_*; Have a look to the Ogg readme file.
  gl_cellshade; 0/1 (0) Cartooned
  gl_cellshade_width; ufff (4)
  gl_bloom;
  con_font; 
  con_font_size;
  font_size;
  alt_text_color;
  Enjoy
  