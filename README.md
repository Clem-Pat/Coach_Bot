
# Coach Bot

CoachBot is a C++ application that controls and gets data from fitness robots. 
The robots use motors to apply a specific torque to the fitness bar so the user can lift it like a regular bar with plate weights. 

This project was the subject of my four-month internship within the Hashimoto Laboratory at the Tokyo University of Science. 

<div align="center">
  <a style="margin: 0 10px;">
    <img src="resources/robot1.jpg" alt="Image 1" height="500"/>
  </a>
  <a style="margin: 0 10px;">
    <img src="https://github.com/Clem-Pat/TrainerBot/blob/main/resources/robot2.jpg" alt="Image 2" height="500"/>
  </a>
</div>

## Goals 

The project aims to estimate the athlete's One-Repetition maximum (OneRM). The OneRM is the maximum load someone can lift in only one repetition. Trying to know one's OneRM requires lifting a hefty load. This can be dangerous for muscles and joints, especially for injured people. This is why we need to estimate it with lighter loads. 

The second goal is to estimate the athlete's Endurance. This is defined as the maximum number of repetitions an athlete can do at a specific load. This data reveals a lot about the athlete's performances and improvements.  


## Screenshot of the application's home page
![ImageApp](resources/homePage2.png)


## Technical brief description

The architecture of the project has a tentacular shape.   
It is separated into six main C++ classes that will interact with one another through the head class Trainer.


## Class Diagram
![Class Diagram](https://github.com/Clem-Pat/TrainerBot/blob/main/resources/ClassDiagram_1.1.1.png)



## Project Report
<a href="resources/Clement_PATRIZIO_Internship_report.pdf">
  <img src="https://static.vecteezy.com/system/resources/previews/023/234/824/original/pdf-icon-red-and-white-color-for-free-png.png" alt="Guide Image" width="70" height="70">
</a>


## Acknowledgement 

I am incredibly grateful to Professor Takuya Hashimoto, who trusted me and welcomed me into his laboratory at the Tokyo University of Science. His support gave me the opportunity to gain valuable experience in a country that was entirely new to me. Under his advice, I enhanced my technical skills in both development and robotics. Moreover, I discovered the unique and fascinating Japanese work culture, which enriched my personal and professional growth. This experience has been exceptional, and I owe much of it to his mentorship.

<div align="center">
  <a href="https://www.ensta-bretagne.fr" style="margin: 0 10px;">
    <img src="https://www.ensta-paris.fr/profiles/createur_profil/themes/createur/dist/images/logo_ensta_new.png.pagespeed.ce.4UezKLM3l6.png" alt="Logo ENSTA" width="300">
  </a>
  <a href="https://www.tus.ac.jp/en/" style="margin: 0 10px;">
    <img src="https://upload.wikimedia.org/wikipedia/en/d/dd/Tokyo_University_of_Science.svg" alt="Logo Tokyo University of Science" width="100">
  </a>
</div>

## Authors

- [@Clem-Pat](https://www.github.com/Clem-Pat)


