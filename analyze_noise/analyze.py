#!/usr/bin/env python
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

def read_data(path):
  df = pd.read_csv(path, comment="#", sep="\t",
      header=None, names=["timestamp", "duration"])
  
  # Convert everything to us
  df /= 1E3
  
  # The benchmark uses different thresholds for recording detours
  # Remove detours < 180ns (worst-case threshold used by benchmark) 
  # Truncate to 30s, skip 1st second as warmup
  if df['timestamp'].max() < 31E6 :
    print "Not enough data, runs must be at least 31s long"
    quit()
  df.drop(df[(df.timestamp < 1E6) |  # Between 1s
                  (df.timestamp > 31E6) | # and 31s (total 30s)
                  (df.duration < 0.180)].index, # and >180 ns
          inplace=True)

  # Reset timestamps
  df.timestamp -= df.timestamp.min()

  return df

# Animate one dataframe
def animate_full(df, name, maxes):
  print "Animating " + name
  # Frames/s
  frame_rate = 1E6 / 5

  nframes = int((df.timestamp.max()) / frame_rate)

  out_dir = "Graphs/" + name
  if not os.path.exists(out_dir):
    os.makedirs(out_dir)

  for fnum in range(nframes):
    # Dataframe for this animation frame
    f_df = df[df.timestamp < (fnum * frame_rate)]
    plot_full(
      {name:f_df},
      None,
      maxes)
      # {'duration':df.duration.max(), 'timestamp':df.timestamp.max()}) 
    
    plt.savefig(out_dir + "/f" + str(fnum) + ".png", format="png")
#    plt.close()


def plot_full(dfs, sums, maxes):

  figs = {}

  for i,k in enumerate(dfs):
    df = dfs[k]
    df_scale = df

    figs[k] = plt.figure(k)

    # Use seconds for timestamp
    df_scale['timestamp'] /= 1E6

    plt.scatter(
      df_scale['timestamp'],
      df_scale['duration'],
      marker='x',
      s=10)
    plt.title(k)
    plt.ylabel("Detour Duration (us)")
    plt.xlabel("Timestamp (s)")
    plt.xlim(xmin=0, xmax=((maxes['timestamp'] * 1.1) / 1E6))
    plt.ylim(ymin=0, ymax=maxes['duration']*1.1)

  return figs

def plot_hist(dfs, sums, maxes):
  figs = {}
  for i,k in enumerate(dfs):
    df = dfs[k]
    figs[k] = plt.figure(k)
    hist = plt.hist(
      df['duration'].values,
      bins=1000,
      range=(0, maxes['duration']),
      # range=(0, 50),
      log=True,
      color='r',
      edgecolor="r",
      weights=np.zeros_like(df['duration'].values) + 1. / 30)

    plt.title(k, size=20)
    plt.xlabel("Duration (us)", size=20)
    plt.ylabel("Detour/S", size=20)
    ax = plt.gca()
    ax.tick_params(axis="x", labelsize=20)
    ax.tick_params(axis="y", labelsize=20)
    #XXX This is hacky, but annoying to fix
    # plt.ylim(ymax=1E3)
    plt.ylim(ymax=200)
    plt.text(0.55, 0.95, sums[k],
        verticalalignment="top", horizontalalignment="left",
        transform=plt.gca().transAxes,
        family="monospace", fontsize=20)
    # plt.annotate("Mean", (df.duration.mean(), 1E-2),
    #     xytext=(15, 1),
    #     fontsize=20,
    #     arrowprops=dict(
    #       facecolor='black',
    #       width=1,
    #       headwidth=5)
    #     )
    # plt.axvline(x=df.duration.mean(), color="black", linewidth=1)
    # plt.vlines(x=df.duration.mean(), ymax=1E2, ymin=1E-2)
    plt.tight_layout()

  return figs

# Returns a dictionary of strings representing a summary of the data
def summarize(dfs):
  sums = {}
  for k in dfs:
    df = dfs[k]
    sums[k] = ""
    sums[k] += "Mean:   " + ("%.2f" % df.duration.mean()) + "\n"
#    sums[k] += "Median: " + ("%.2f" % df.duration.median()) + "\n"
    sums[k] += "STD:    " + ("%.2f" % df.duration.std()) + "\n"
#    sums[k] += "Min:    " + ("%.2f" % df.duration.min()) + "\n"
#    sums[k] += "Max:    " + ("%.2f" % df.duration.max()) + "\n"
#    sums[k] += "99%:    " + ("%.2f" % df.duration.quantile(0.9) ) + "\n"
    sums[k] += "99.9%:  " + ("%.2f" % df.duration.quantile(0.99) ) + "\n"
    # sums[k] += "Det/s:  " + ("%.2f" % (df.timestamp.max() / df.duration.count())) + "\n"
    sums[k] += "Det/s:  " + ("%.2f" % (df.duration.count() / df.timestamp.max()*1E6)) + "\n"
    sums[k] += "Waste:  " + ("%.2f" % (df.duration.sum()*100 / df.timestamp.max())) + "%\n"

  return sums

# This directory will be searched for results to use
RESULT_DIR = "named_results/"

def main():

  # paths = ["Example 1", "Example 2"]
  fnames = os.listdir(RESULT_DIR)
  dfs = {}
  for fname in fnames:
    path = RESULT_DIR + fname
    if os.path.isfile(path):
      dfs[fname] = read_data(path)

  # Get the global min/max for plotting
  # You may want to tweak this to make result smore readable or consistent
  maxes = {}
  maxes['timestamp'] = max([dfs[k].timestamp.max() for k in dfs])
  # maxes['timestamp'] = 30
  maxes['duration']  = max([dfs[k].duration.max() for k in dfs])
  # maxes['duration'] = 50

  sums = summarize(dfs)
  for k in sums:
    print k
    print sums[k]

  # for k in dfs:
  #   animate_full(dfs[k], k, maxes)
  
  figs = plot_hist(dfs, sums, maxes)
  # figs = plot_full(dfs, sums, maxes)
  for k in figs:
    figs[k].savefig("plots/" + k + ".pdf", format="pdf")

  #plt.show()

main()
