git add .
git commit -m "add comment"
if [ -n "$(git status - porcelain)" ];
then
 echo "IT IS CLEAN"
else
 echo "Pushing data to remote server!!!"
 git push -u origin master
fi