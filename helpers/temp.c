char path[256];
		FILE *file;
		char line[256];
		
		// Read from /proc/[pid]/status for detailed info
		snprintf(path, sizeof(path), "/proc/%d/status", pid);
		file = fopen(path, "r");
		
		if (file == NULL) {
			printf("Process %d no longer exists\n", pid);
			// Remove from list since process is dead
			head = deleteNode(head, pid);
			return;
		}
		
		printf("=== Process %d Statistics ===\n", pid);
		
		// Read and display key information
		while (fgets(line, sizeof(line), file)) {
			// Print specific fields we care about
			if (strncmp(line, "Name:", 5) == 0 ||
				strncmp(line, "State:", 6) == 0 ||
				strncmp(line, "PPid:", 5) == 0 ||
				strncmp(line, "VmSize:", 7) == 0 ||
				strncmp(line, "VmRSS:", 6) == 0) {
				printf("%s", line);
			}
		}
		fclose(file);
		
		// Also read from /proc/[pid]/stat for additional info
		snprintf(path, sizeof(path), "/proc/%d/stat", pid);
		file = fopen(path, "r");
		if (file) {
			long utime, stime;
			// Fields: pid comm state ppid ... utime stime ...
			// We'll skip to the timing fields (positions 14 and 15)
			char comm[256], state;
			int ppid;
			fscanf(file, "%*d %s %c %d %*d %*d %*d %*d %*u %*u %*u %*u %*u %ld %ld", 
				   comm, &state, &ppid, &utime, &stime);
			printf("CPU time (user): %ld clock ticks\n", utime);
			printf("CPU time (system): %ld clock ticks\n", stime);
			fclose(file);
		}
		
		printf("=============================\n");
		
	} else {