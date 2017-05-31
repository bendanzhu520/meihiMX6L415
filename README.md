 Command line instructions
Git global setup

git config --global user.name "Administrator"
git config --global user.email "admin@example.com"

Create a new repository

git clone ssh://git@192.168.9.247:18083/meihuan/meihiMX6L4115.git
cd meihiMX6L4115
touch README.md
git add README.md
git commit -m "add README"
git push -u origin master

Existing folder or Git repository

cd existing_folder
git init
git remote add origin ssh://git@192.168.9.247:18083/meihuan/meihiMX6L4115.git
git add .
git commit
git push -u origin master
