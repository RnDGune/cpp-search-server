# cpp-search-server
### Спринт 1-8, финальный проект: поисковая система 
#### Проект ядра поискового сервера с поддержкой стоп-слов и минус слов.  
Стоп-слова игнорируются при формировании запроса, минус-слова исключают из выдачи все документы где они встречаются.  
Ранжирование результата происходит по TF-IDF, при равенстве - по рейтингу документа (задается при добавлении документа).  
Методы поиска документов по запросу имеют последовательную и параллельнуе версии.  

В репоизитории лежит проект с main.cpp сделаным для тестирования.  
```cpp

SearchServer search_server("stop words"s); // создание обьекта search_server со стоп словам "stop words"
search_server.AddDocument(id,"text of document"s,DocumentStatus::STATUS,{raiting}) // добавление документов в поисковый сервер
document = search_server.FindTopDocuments(execution::seq,"text to find -stop -word"s,DocumentStatus::STATUS); // поиск документа содержащего  
//"text to find" со сатусом STATUS и минус словами stop и word
PrintDocument(document); // вывести найденый документ
```
id -id документа int, text of document - текст string, 
DocumentStatus - статус ACTUAL,IRRELEVANT, BANNED,REMOVED,
raiting - рейтинг документа, задается набором int'ов например:{1,3,0};
