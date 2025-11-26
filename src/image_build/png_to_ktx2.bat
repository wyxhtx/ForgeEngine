.\toktx --t2 --zcmp 22 --assign_primaries bt709 --assign_oetf srgb ../image/terrain/cube_16_64.ktx2 terrain/cube_16_64.png
.\toktx --t2 --zcmp 22 --assign_primaries bt709 --assign_oetf linear ../image/terrain/cube_16_64_specular.ktx2 terrain/cube_16_64_specular.png

.\toktx --t2 --encode uastc --zcmp 22 --genmipmap --assign_primaries bt709 --assign_oetf srgb ../image/model/temple/temple.ktx2 model/temple/temple.png