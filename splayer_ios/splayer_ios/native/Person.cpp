#include "Person.hpp"

Person::Person(){
    age = 0;
    sex = true;
    name = "who?";
}

Person::Person(const char* name , const int age , const bool sex){
    this->age = age;
    this->sex = sex;
    
    long len = strlen(name);
    char * cpname = new char[len + 1];
    strcpy(cpname, name);
    this->name = cpname;
}

Person::~Person(){
    cout << "person destruct\n";
}

void Person::introduceMySelf(){
    cout << "hello , i am " << name << ", my age is " << age << "year";
    if (sex) {
        cout << "i am boy";
    }else{
        cout << "i am girl";
    }
}
