import os
import sys
import shutil
import subprocess

def main():
	for f in os.listdir(sys.argv[1]):
		if f.startswith('video'):
			parts = f.split()
			newName = '_'.join(parts)
			path = sys.argv[2] + newName
			shutil.copyfile(sys.argv[1] + f, path)
			iden = f.split('.')[0]
			outputPath = sys.argv[2] + iden
			if not os.path.exists(outputPath):
				os.mkdir(outputPath)
				subprocess.call(['turkic', 'extract', path, outputPath])

				subprocess.call(['turkic', 'load', iden, outputPath, 'arm'])

main()
