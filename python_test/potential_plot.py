import numpy as np
import matplotlib.pyplot as plt

'''
Program to plot a 2D version of the ESP from PB-AM
'''
fileName='/Users/lfelberg/Desktop/test/electrostatic_test/out.x.0.dat'
#fileName='/Users/lfelberg/Desktop/pot_z_0.00.dat'
outFile='/Users/lfelberg/Desktop/pot_x_0.00.jpg'

#-----------------------------------------------------------------------
def FileOpen(fileName):
    """Gets data from 2D plot output of PB-AM"""
    lines = open(fileName).readlines()
    
    grid,org,dl = np.zeros(2), np.zeros(2),np.zeros(2)
    axval, ax, units, mn, mx = 0.0, 'x', 'jmol', 0.0, 0.0
    pot = np.zeros((100,100))
    ct = 0

    for line in lines:
        temp = line.split()
        if 'units' in line[0:10]:
            units = temp[1]
        elif 'grid' in line[0:10]:
            grid[0], grid[1] = int(temp[1]), int(temp[2])  
            pot = np.zeros((grid[0], grid[1]))
        elif 'axis' in line[0:10]:
            ax, axval = temp[1], float(temp[2])
        elif 'origin' in line[0:10]:
            org[0], org[1] = float(temp[1]), float(temp[2])  
        elif 'delta' in line[0:10]:
            dl[0], dl[1] = float(temp[1]), float(temp[2]) 
        elif 'maxmin' in line[0:10]:
             mx, mn = float(temp[1]), float(temp[2]) 
        elif '#' not in line:
            temp = [float(x) for x in line.split()]

            for i in range(int(grid[0])):
                pot[ct][i] = temp[i]
            ct += 1

    return(pot, org, dl, ax, axval, units, mx, mn)

def dispPlot( org, bn, count, potential, 
                mx = 0.1, mn = -0.1, title = '', 
                xlab = r'$X \AA$', ylab = r'$Y \, (\AA)$',
                lege = '', outFile = None ):
    """Plots the colormap of potential plot, 2D"""
    fig = plt.figure(1, figsize = (5, 4)); 
    ax = fig.add_subplot(1,1,1)

    nbins = len(potential[0])
    #X, Y = np.meshgrid(np.arange(0, 100, 1), np.arange(0, 100, 1))
    #levels = np.arange(-1,1,0.1)
    #plt.contourf(X, Y, potential, levels)
    
    X = np.arange(org[0], org[0]+ nbins*bn[0], bn[0])
    Y = np.arange(org[1], org[1]+ nbins*bn[1], bn[1])
    plt.pcolor(X, Y, potential, vmin=mn+0.1*mn, vmax=mx)
    plt.colorbar()

    ax.set_xlim([X[0], X[-1]])
    ax.set_ylim([Y[0], Y[-1]])
    plt.title(title, fontsize = 13);
    ax.set_ylabel(ylab, fontsize = 10); 
    ax.set_xlabel(xlab, fontsize = 10)
    for tick in ax.xaxis.get_major_ticks():
        tick.label.set_fontsize(8)
    for tick in ax.yaxis.get_major_ticks():
        tick.label.set_fontsize(8)
    if outFile != None:
        plt.savefig(outFile,bbox_inches='tight', dpi = 200)
    plt.close()

#--------------------------------------------------------------------------------
# main

esp, org, dl, ax, axval, units, mx, mn = FileOpen(fileName)
boxl = len(esp[0])

titl = 'Cross section at ' + ax + ' = ' + str(axval)
titl += ' in ' + units
xla = r'$Y \, (\AA)$'; yla = r'$Z \, (\AA)$'
if ax == 'y':
    xla = r'$X \, (\AA)$'; yla = r'$Z \, (\AA)$'
elif ax == 'z':
    xla = r'$X \, (\AA)$'; yla = r'$Y \, (\AA)$'

esp[ esp == 0.0] = None
 
dispPlot( org, dl, len(esp[0]), np.transpose(esp), 
                mx, mn, titl,
                xlab = xla, ylab = yla,
                outFile=outFile)