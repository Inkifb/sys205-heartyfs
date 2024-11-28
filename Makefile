all:
	gcc -o bin/heartyfs_init src/heartyfs_init.c;
	gcc -o bin/heartyfs_mkdir src/op/heartyfs_mkdir.c src/heartyfs_bitmap.c src/heartyfs_util.c;
	gcc -o bin/heartyfs_rmdir src/op/heartyfs_rmdir.c src/heartyfs_bitmap.c src/heartyfs_util.c;
	gcc -o bin/heartyfs_creat src/op/heartyfs_creat.c src/heartyfs_bitmap.c src/heartyfs_util.c;
	gcc -o bin/heartyfs_rm src/op/heartyfs_rm.c src/heartyfs_bitmap.c src/heartyfs_util.c;
	gcc -o bin/heartyfs_read src/op/heartyfs_read.c  src/heartyfs_bitmap.c src/heartyfs_util.c;
	gcc -o bin/heartyfs_write src/op/heartyfs_write.c src/heartyfs_bitmap.c src/heartyfs_util.c;

clean:
	rm -rf bin/*
