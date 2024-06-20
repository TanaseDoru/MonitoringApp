# MonitoringApp
Aplicatie de monitorizare resurse
# 17.06.2024
Am facut o scurta cercetare asupra unor teme si m-am stabilit pe tema de Monitoring Folosind ncurses
# 18.06.2024
Commits: Basic Menu; Show Specs Initial
Am scris un meniu prin care sa se poata selecta: componente HW si Procese actuale
Am facut headere si fisiere separate pentru fiecare functionalitate
Am facut un meniu adecvat pentru optiunea de "Show Specs" prin care se poate rula folosind tasta TAB
# 19.06.2024
Commits: Summary Done; Show Specs Done
Am terminat primul tab din meniul Show Specs, astfel incat sa afiseze niste detalii scurte despre componentele calculatorului
Am finalizat tab-ul de Show Specs, astfel sa arate date despre componentele calculatorlui utilizatorului cum ar fi: 
Date despre CPU, Memorie RAM, Storage, Periferice.
Pentru meniuri mai lungi, cum ar fi periferice am implementat functionalitatea de a da scroll folosind scroll-ul de la mouse sau sageti de la tastatura

# 20.06.2024
Commits: Beggining Processes
Am inceput lucru pentru Tab-ul de Monitor Processes.
Am extras procesele din fisierele aferente din /proc
Am afisat date despre procese si anume: State, PID, PPID, Owner, CMD
Am facut 2 ecrane: Cel de sus pentru detalii, iar cel de jos pentru procesele efective
Procesle pot fi evidentiate, mai tarziu putand fi omorate
Am mai adaugat o functionalitate prin care terminalul trebuie sa aiba o anumita marime pentru ca utilizatorul sa poata interactiona cu aplicatia.
