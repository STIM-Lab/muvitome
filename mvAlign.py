# -*- coding: utf-8 -*-
"""
Created on Mon Jun 14 12:13:02 2021

@author: Jack
"""

import skimage.io
import numpy as np
import scipy as sp
import matplotlib.pyplot as plt
from os import listdir
from os.path import isfile, join
import os
from tqdm import tqdm
import mvCorrect as mvc


def OffsetFunction(a, b, offset):
    
    # if the function a is shifted left
    if offset < 0:
        offset_a = a[-offset:]
        offset_b = b[:len(b) + offset]
        
    # if the function a is shifted right (then b is shifted left)
    elif offset > 0:
        offset_b = b[offset:]
        offset_a = a[:len(a) - offset]
    else:
        offset_a = a.copy()
        offset_b = b.copy()
        
    return offset_a, offset_b
    


# This function calculates the error in the alignment between two 1D functions
def AlignmentError1D(a, b, sigma, offset):
    
    # make local copies of the 1D functions
    local_a = a.copy()
    local_b = b.copy()
    
    # mean correct both functions
    local_a_mean = np.mean(local_a)
    local_b_mean = np.mean(local_b)
    local_a = local_a - local_a_mean
    local_b = local_b - local_b_mean
    
    # if a blur deviation is specified, blur the functions to compensate for noise
    if sigma > 0:
        local_a = sp.ndimage.gaussian_filter(local_a, sigma)
        local_b = sp.ndimage.gaussian_filter(local_b, sigma)
    
    off_a, off_b = OffsetFunction(local_a, local_b, offset)
    
    
    # calculate the difference between both functions    
    diff = off_a - off_b
    
    # calculate the absolute difference
    abs_diff = np.abs(diff)
    
    # sum the absolute differences to get an error metric
    mean_diff = np.mean(abs_diff)
    
    return mean_diff


# calculates the number of pixels that image A is offset from image B along the y-axis   
def ImageOffsetY(A, B, N, sigma=0):
    profileA = np.mean(A, axis=1)
    profileB = np.mean(B, axis=1)

    error = np.zeros(2 * N)
    for i in range(-N, N):
        error[i + N] = AlignmentError1D(profileA, profileB, sigma, i)
        
    min_error = np.argmin(error)
    return min_error - N

def ApplyImageOffsetY(A, B, offset):
    
    # if image a is shifted up
    if offset < 0:
        offset_A = A[-offset:, :]
        offset_B = B[:B.shape[0] + offset, :]
        
        
    return offset_A, offset_B

# calculates a list of offsets along the Y axis for each image in a directory
def CalculateImageOffsetListY(directory, N=100, sigma=0):
    onlyfiles = [f for f in listdir(directory) if isfile(join(directory, f))]

    relative = []
    relative.append(0)

    # read the first file and store it in A
    A = skimage.io.imread(directory + "/" + onlyfiles[0], as_gray=True)

    # iterate through every other file in the directory
    print("Calculating image offsets...")
    for fi in tqdm(range(1, len(onlyfiles))):
        B = skimage.io.imread(directory + "/" + onlyfiles[fi], as_gray=True)
        
        B_to_A = ImageOffsetY(B, A, N, sigma)
        relative.append(B_to_A)
        
        A = B
        
    # convert from relative to absolute offsets
    absolute = []
    absolute.append(relative[0])

    for i in range(1, len(relative)):
        absolute.append(absolute[i-1] + relative[i])
        
    
    absolute_array = np.array(absolute)
    min_offset = np.min(absolute_array)
    
        
    return absolute_array - min_offset

 
def ApplyImageOffsetsY(in_directory, out_directory, offsets):
    if not os.path.exists(out_directory):
        os.mkdir(out_directory)
    onlyfiles = [f for f in listdir(in_directory) if isfile(join(in_directory, f))]

    # load the first image to get the image size
    I = skimage.io.imread(in_directory + "/" + onlyfiles[0])

    # calculate the dimensions of the final array
    dims = np.array(I.shape)
    max_offset = np.max(offsets)
    dims[0] = dims[0] + max_offset

    for fi in tqdm(range(len(onlyfiles))):
        I = skimage.io.imread(in_directory + "/" + onlyfiles[fi])
        Iout = np.zeros(dims, I.dtype)
        start_index = offsets[fi]
        end_index = offsets[fi] + I.shape[0]
        Iout[start_index:end_index, :, :] = I
        skimage.io.imsave(out_directory + "/" + onlyfiles[fi], Iout)
    
    
    

   

