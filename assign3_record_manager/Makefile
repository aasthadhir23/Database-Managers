all:
	gcc -w buffer_mgr.c buffer_mgr_stat.c dberror.c storage_mgr.c expr.c record_mgr.c rm_serializer.c test_assign3_1.c -o test_assign3_1
	./test_assign3_1

expr:
	gcc -w buffer_mgr.c buffer_mgr_stat.c dberror.c storage_mgr.c expr.c record_mgr.c rm_serializer.c test_expr.c -o test_expr
	./test_expr

clean:
	$(RM) test_assign3_1
	$(RM) test_expr
