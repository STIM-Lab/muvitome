import numpy as np
import os
from tqdm import tqdm
import skimage
import skimage.transform
import shutil

def AssembleMuvitome(in_path, out_path, overlap_percent = (0, 0)):
    # get the list of folders to determine the mosaic size
    TileFolders = os.listdir(in_path)
    
    if not os.path.exists(out_path):
        os.mkdir(out_path)
    
    # if there aren't any tile folders, just copy the images
    if os.path.isfile(in_path + "/" + TileFolders[0]):
        ImageNames = os.listdir(in_path)
        for i in range(len(ImageNames)):
            shutil.copyfile(in_path + "/" + ImageNames[i], out_path + "/" + ImageNames[i])
        return
    
    


    # calculate the number of rows and columns
    Columns = []
    Rows = []

    
    for f in TileFolders:
        indices = [int(s) for s in f.split("_") if s.isdigit()]
        Columns.append(indices[0])
        Rows.append(indices[1])
        
    NumColumns = np.max(Columns) - np.min(Columns)
    NumRows = np.max(Rows) - np.min(Rows)
    
    Columns = Columns - np.min(Columns)
    Rows = Rows - np.min(Rows)

    # get the names of all images in the stack
    ImageNames = os.listdir(in_path + "/" + TileFolders[0])
    
    print("Assembling images")

    # for each image in the stack
    NumImages = len(ImageNames)
    for i_name in tqdm(range(NumImages)):
        imagename = ImageNames[i_name]
        
        Tiles = []
        for tilefolder in TileFolders:
            tile = skimage.io.imread(in_path + "/" + tilefolder + "/" + imagename)
            Tiles.append(tile)
          
        # get the size of the images
        InputTileWidth = Tiles[0].shape[1]
        InputTileHeight = Tiles[0].shape[0]
        OverlappedTileWidth = int(InputTileWidth * ((100 - overlap_percent[0]) / 100))
        OverlappedTileHeight = int(InputTileHeight * ((100 - overlap_percent[1]) / 100))
        OverlapWidth = InputTileWidth - OverlappedTileWidth
        OverlapHeight = InputTileHeight - OverlappedTileHeight
        TileColors = Tiles[0].shape[2]
        
        
        # calculate the size of the stitched image    
        Width = (NumColumns) * OverlappedTileWidth + InputTileWidth
        Height = (NumRows) * OverlappedTileHeight + InputTileHeight
        
        # allocate and assemble the stitched image
        Stitched = np.zeros((Height, Width, TileColors))
        #Overlays = np.zeros((Height, Width)).astype(np.uint8)
        
        for i in range(len(Tiles)):
            ir = Rows[i] * OverlappedTileHeight
            ic = Columns[i] * OverlappedTileWidth
            Patch = Stitched[ir:ir+InputTileHeight, ic:ic+InputTileWidth, :]
            #Patch = Patch + np.fliplr(Tiles[i])
            Patch = np.maximum(Patch, np.fliplr(Tiles[i]))
            
            Stitched[ir:ir+InputTileHeight, ic:ic+InputTileWidth, :] =  Patch
            #Overlays[ir:ir+InputTileHeight, ic:ic+InputTileWidth] += 1
        #Overlays = (Overlays - 1) * 0.5 + 1
        #for c in range(TileColors):
        #    Stitched[:, :, c] = Stitched[:, :, c] / (Overlays)
        skimage.io.imsave(out_path + "/" + imagename + ".tif", Stitched.astype(np.uint8))
            
        
        
