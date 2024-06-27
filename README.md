# MonitoringApp
* Aplicatie de monitorizare resurse
# 17.06.2024
* Am facut o scurta cercetare asupra unor teme si m-am stabilit pe tema de Monitoring Folosind ncurses
# 18.06.2024
* Commits: Basic Menu; Show Specs Initial. 
* Am scris un meniu prin care sa se poata selecta: componente HW si Procese actuale
* Am facut headere si fisiere separate pentru fiecare functionalitate
* Am facut un meniu adecvat pentru optiunea de "Show Specs" prin care se poate rula folosind tasta TAB
# 19.06.2024
* Commits: Summary Done; Show Specs Done. 
* Am terminat primul tab din meniul Show Specs, astfel incat sa afiseze niste detalii scurte despre componentele calculatorului
* Am finalizat tab-ul de Show Specs, astfel sa arate date despre componentele calculatorlui utilizatorului cum ar fi: 
* Date despre CPU, Memorie RAM, Storage, Periferice.
* Pentru meniuri mai lungi, cum ar fi periferice am implementat functionalitatea de a da scroll folosind scroll-ul de la mouse sau sageti de la tastatura

# 20.06.2024
* Commits: Beggining Processes. 
* Am inceput lucru pentru Tab-ul de Monitor Processes.
* Am extras procesele din fisierele aferente din /proc
* Am afisat date despre procese si anume: State, PID, PPID, Owner, CMD
* Am facut 2 ecrane: Cel de sus pentru detalii, iar cel de jos pentru procesele efective
* Procesle pot fi evidentiate, mai tarziu putand fi omorate
* Am mai adaugat o functionalitate prin care terminalul trebuie sa aiba o anumita marime pentru ca utilizatorul sa poata interactiona cu aplicatia.

# 21.06.2024
* Commits: Processes efficient. 
* Am eficientizat monitorizarea proceselor si extragerea acestora din fisiere folosind threads
* De acum toate procesele sunt stocate intr-o clasa, care se actualizeaza la minim 2 secunde, dupa ce se apasa o tasta

# 25.06.2024
* Commits: Summary init
* Am implementat un mod de cautare pentru procese scriind numele acestuia(momentan doar scrie dar nu face si cautarea)
* Am afisate date in partea de sus
* 
# 26.06.2024
* Commits: Nearly Done
* Am facut ca search sa functioneze cum trebuie.
* Am implementat functionalitatea de terminate process
* Am implementat functionalitatea de Nice + si Nice -
* Am rezolvat o problema prin care fisierele nu se inchideau, rezultand in multiple probleme prin care nu se putea executa comenzi sau deschidere fisiere 
* Am adaugat o noua 'fereastra': In momentul in care se apasa sageata dreapta, se va afisa doar numele comenzii, precum si argumentele acestia
* Am adaugatt functionalitatea de resize si pentru meniul de Show Specs

# 27.06.2024
* Commits: Done
* Am facut meniul de Help din Process Manager
* Mici detalii cum ar fi spatiere
