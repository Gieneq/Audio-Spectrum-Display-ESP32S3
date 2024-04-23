import numpy as np

def calculate_bin_edges(num_bins, num_samples, scale='log', skew=1.6, min_value=2):
    if scale == 'log':
        max_value = num_samples
        
        edges = np.linspace(0, 1, num_bins+1, dtype=float)
        edges = edges**skew
        edges = min_value + edges * (max_value - min_value)
        edges = np.unique(np.floor(edges).astype(int))

    elif scale == 'linear':
        # Generate linearly spaced edges
        edges = np.linspace(0, num_samples, num_bins + 1, dtype=int)
    else:
        raise ValueError("Unsupported scale type")

    # Ensure we cover all frequencies up to num_samples / 2
    if edges[-1] < (num_samples):
        edges[-1] = num_samples

    return edges

def assign_samples_to_bins(num_bins, num_samples, skew=1.6, min_value=2):
    # Calculate the bin edges
    bin_edges = calculate_bin_edges(num_bins, num_samples, scale='log', skew=skew, min_value=min_value)
    print(len(bin_edges), bin_edges)

    # Create a list of tuples indicating the range of FFT samples for each bin
    bins = [(bin_edges[i], bin_edges[i + 1]) for i in range(len(bin_edges) - 1)]
    print(len(bins))

    return bins

# Example usage
num_bins = 21
num_samples = 1024
min_value = 3
bins = assign_samples_to_bins(num_bins, num_samples, skew=1.6, min_value=min_value)
bins_map = []
bins[0] = (bins[0][0] - min_value, bins[0][1]) 

print("Bin ranges (start, end):")
for idx, (start, end) in enumerate(bins):
    bins_count = end - start
    bin_map = [idx] * bins_count
    bins_map.extend(bin_map)
    print(f"{idx}: {start} to {end}, count={bins_count}")
    # print(f"{idx}: {start} to {end}, count={bins_count}, bins:{bin_map}")
# bins_map.append(num_bins-1)
print(f"static const uint8_t bins_map[{len(bins_map)}] = ", "{", ", ".join(str(bin) for bin in bins_map), "};", sep="")