# -*- coding: utf-8 -*-
"""
Created on Fri Apr 26 11:22:16 2024

@author: david
"""

import PIL
from os import listdir
from os.path import isfile, isdir, join
from tqdm import tqdm


def TiffConvert(source_directory, dest_directory):

    # load the image stack
    frame_directories = [f for f in listdir(source_directory) if isdir(join(source_directory, f))]
    
    frame_num = 1
    for frame_directory in frame_directories:
        print("")
        print("Processing stack " + str(frame_num) + "/" + str(len(frame_directories)))
        
        dir_input = source_directory + "/" + frame_directory
        
        # load the image stack
        image_files = [f for f in listdir(dir_input) if isfile(join(dir_input, f))]
        
        image_list = []
        for fi in tqdm(range(len(image_files))):
            image_list.append(PIL.Image.open(dir_input + "/" + image_files[fi]))
        
        print("Saving stack " + str(frame_num) + "...", end=" ")
        image_list[0].save(dest_directory + "/" + str(frame_num) + ".tif", compression="tiff_deflate", save_all=True,
                       append_images=image_list[1:])
        print("done.")
        
        frame_num = frame_num + 1