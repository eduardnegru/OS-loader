***************************************README*************************************************
Negru Adrian Eduard
332CC

https://gitlab.cs.pub.ro/adrian_eduard.negru/tema3SO


In functia so_init_loader setez handler-ul(segv_handler) care trebuie apelat
in cazul in care se produce un seg fault (SIGSEGV).

In handler trec prin toate segmentele si verific daca adresa care a cauzat
seg fault-ul se gaseste in unul din segmente. Daca nu se gaseste atunci rulez 
handlerul default de seg fault. Verifica apoi daca adresa(obtinuta din campul si_addr al
structurii siginfo_t) este mapata este mapata. Pentru a verifica daca adresa 
este mapata folosesc campul si_code din structura siginfo_t, egal cu 
SEGV_MAPERR in cazul in care adresa nu este mapata, sau egaul cu 
SEGV_ACERR in cazul in care adresa este mapata, dar nu exista suficiente
permisiuni pentru accesa memoria. Daca si_code = SEGV_ACERR atunci rulez
handlerul default de seg fault, altfel daca si_code = SEGV_MAPERR incerc sa 
mapez pagina.

Aliniez adresa care a cauzat seg fault-ul cu ALIGN_DOWN. Mapez in memorie o pagina
de page_size cu MAP_FIXED si MAP_PRIVATE, iar in cazul in care file_size < mem_size
folosesc MAP_ANONYMOUS | MAP_PRIVATE. (map_anon este folosit pentru ca nu mai am 
date in fisier pe cares sa se bazeze maparea si pentru ca datele sunt zeroizate).

Apelez un memset pe pagina pe care am mapat-o pentru a ma asigura ca in cazul in care
adresa de la sfarsitul fisierului este mai mica decat adresa de la sfarsitul paginii
voi avea memorie zeroizata. Daca sunt in acest caz, copiez din fisier
file_size_end_address - alignedAddress bytes. Altfel copiez din fisier page_size bytes.
Pentru a copia din fisier folosesc lseek pentru a ajunge la offset-ul 
crespunzator in fisier, citesc intr-un buffer si apoi copiez cu memcpy.

Dupa copierea datelor setez permisiunile corespunzatoare pentru pagina mapata cu mprotect.

