CODE_VERSION=$(git rev-list HEAD --first-parent --count)
echo "#ifndef CARCOMPUTER_VERSION_H
#define CARCOMPUTER_VERSION_H

#define APP_VERSION \"$CODE_VERSION\"

#endif //CARCOMPUTER_VERSION_H" > ./main/version.h
idf.py -p /dev/ttyUSB0 flash monitor