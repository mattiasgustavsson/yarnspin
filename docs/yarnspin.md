
![](../images/yarnspin_logo.png)

# User Handbook

---

1.Introduction
--------------

Welcome to the world of *Yarnspin*, a friendly and approachable game engine designed to help you create your very own choose-your-own-adventure story games! Drawing inspiration from home computer game development tools from the 1980s and 1990s, Yarnspin aims to provide an accessible and enjoyable experience, whether you're a seasoned developer or just starting out. In this introductory chapter, we'll cover the essential information you need to get started with Yarnspin, including system requirements, how to make the most of this manual, and what you'll find in the Yarnspin package. So, let's dive in!


### System Requirements

Yarnspin games will run on Windows, MacOS, Linux and web browsers.

The Yarnspin development tools runs on Windows, MacOS and Linux, but it is not possible to use a web browser to develop Yarnspin games. 

In addition to the Yarnspin tools, you will also need a text editor - any text editor will work, as long as it can save plain text files. Sublime (www.sublimetext.com) is a popular choice that runs on all three operating systems.

It might also be useful to have a paint program or image editor, as well as sound and music programs.


### Using This Manual

This manual is designed to be your comprehensive guide to Yarnspin, providing you with all the information you need to create engaging choose-your-own-adventure story games. 

In addition to explaining key concepts, the manual will walk you through the process of creating a simple story game, or "yarn". By breaking down a sample yarn, you'll gain an understanding of how the various components of Yarnspin work together to create an interactive narrative experience.

As you progress through the manual, you'll find numerous examples, tips, and best practices that will help you become proficient in using the Yarnspin engine. 


### What's in the package

When you open up the Yarnspin package, you'll find a set of folders and files. The folders already contains files used for the tutorial game, but as you make your own yarns, you will replace most of these files with your own content.

Here's a quick overview of what each folder contains:

* `build/`: This folder contains support files used internally by Yarnspin to build stand-alone redistributable packages of your games. Normally, you would never have to touch the files in this folder.
* `faces/`: In here, you will save portrait pictures of all characters in your game. 
* `fonts/`: A collection of fonts to give your game that perfect look and feel. A bunch of fonts are included, but feel free to add your own.
* `images/`: Here you put all images used to represent the different locations of the game.
* `palettes/`: Yarnspin games can use a palette to give the game a distinct look. This folder includes some palette definitions, but you can also add your own.
* `scripts/`: Every part of a yarn is defined in script files, and they are placed in this folder. The names or extension of the files in this folder doesn't matter - they will all be loaded and compiled into a single game.
* `sound/`: Yarnspin games can have music and sound effects, and those are placed in this folder.

And of course, you'll find two essential files:

`yarnspin.exe`: The heart of Yarnspin! This engine processes, compiles, and runs and packages your yarns.

`yarnspin.pdf`: The very documentation you're reading right now, guiding you step by step on your Yarnspin journey.


### About the Author

My journey into game development began in the 1980s with a love for home computers and programming. Starting with a Commodore 64, and later moving on to the Atari ST, I spent countless hours learning to code first in BASIC and later in assembler, making games and running tabletop RPGs with friends.

Later in life, I worked as a game developer, mostly in the engine teams for games like Crackdown, Bodycount and Battlefield 4. In 2014, I decided to leave the professional game development world to return to my roots, focusing on making games as a hobby and fueling my love for retro games.

With Yarnspin, I aim to share my appreciation for game development and empower others to create their own games. Drawing inspiration from the friendly and approachable nature of 80s and 90s game development tools, I hope Yarnspin will enable you to embark on your own creative journey.



2. Getting Started
------------------

In this chapter, we'll help you dive into the world of Yarnspin by guiding you through the sample game, understanding the basic concepts of Yarnspin, and creating your very first yarn. By the end of this chapter, you'll have a solid foundation in Yarnspin and be well on your way to creating interactive stories of your own. 



### Running the Sample

Before you start crafting your own game, it's useful to get familiar with the look and feel of a Yarnspin game by running the provided sample. The tutorial game is a good way to understand how the engine works and visualize the concepts we'll be discussing in the next section.

To play the tutorial game, simply launch the `yarnspin` executable in the root folder of the yarnspin package.

Take your time to explore the tutorial and don't hesitate to revisit it later as you progress through this manual. It will serve as a valuable reference when working on your own projects.


### Basic Concepts

Now that you have experienced a Yarnspin game first-hand, let's discuss some fundamental concepts that will help you understand the underlying structure of Yarnspin projects.


#### Sections
Sections are the building blocks of your Yarnspin game. They define the structure of your story and allow you to organize your game into manageable parts. There are several types of sections, each serving a specific purpose.

#### Location Sections
Location Sections
Location sections represent different places in your game. They serve as the backdrop for the story and give context to the player's actions. Locations can be described in text, and you can use images to further enhance the experience. Options within location sections provide players with choices to navigate through the game world and interact with their surroundings.

#### Dialog Sections
Dialog sections handle the conversations between characters in your game. They enable you to create dynamic and engaging dialogues, allowing the player to interact with and influence the story through their choices. Dialog options give players the ability to respond to other characters, shaping the direction and outcome of the conversation.

#### Character Sections
Character sections define the attributes of each character in the story. They provide a way to store and manage information about the characters, such as their names and images.

#### Globals
Globals are declarations that control the overall appearance and behavior of your game. They include settings like the game's title, author, starting section, fonts, colors, and more. Globals are essential for customizing your game and ensuring a consistent look and feel.

#### Flags
Flags are used to track the state of your game, such as which events have occurred, and can be set, cleared, or toggled. They allow you to create dynamic, interactive experiences by altering the game state based on player choices and actions.

#### Items
Items are objects that the player can collect, use, or drop throughout the game. They play a crucial role in creating a sense of immersion and encouraging exploration within your Yarnspin game.

#### Conditions
Conditions determine whether certain elements should be displayed or activated based on the current state of your game. By using conditions with flags and items, you can create branching paths and interactions that depend on what the player has done or what they possess.

With these core concepts in mind, you'll have a clearer understanding of how Yarnspin games are structured and how the different elements work together. In the next section, we'll guide you through creating your very first yarn, where you'll put these concepts into practice.


### Your First Yarn

In this section, we'll guide you through creating a very simple Yarnspin game from scratch. We'll start by removing the existing scripts and then walk you through writing a small, but functional, script file. The goal is to create a single location with a single dialog to help you understand the basic structure of a Yarnspin game.


#### Step 1: Removing Existing Scripts

Before you start creating your own game, make sure to remove any existing scripts in the 'scripts' folder. This will ensure that your new script won't conflict with any pre-existing files.


#### Step 2: Creating a New Script File

Create a new text file in the 'scripts' folder and name it "my_first_yarn.txt". This file will serve as the script for your simple Yarnspin game.


#### Step 3: Defining Globals

At the top of the "my_first_yarn.txt" file, define the following globals:

```
title: My First Yarn
author: Your Name
start: my_location
```

This sets the title, author, and starting location for your game.


#### Step 4: Creating a Location Section

Now, let's create a simple location section. Add the following code to your script file:

```
=== my_location ===

img: office.jpg
txt: You are standing in a small room. There is a person here, it's Carol.

opt: Talk to Carol.
act: my_dialog
```

This code defines a location called "my_location" with an image and description. There's also an option for the player to talk to Carol, which will lead to a dialog section.


#### Step 5: Creating a Character Section

Before defining a dialog with a character, we need to create a character section. Add the following code to your script file:

```
=== carol ===

name: Carol the Neighbor
short: Carol
face: carol.jpg
```

This code defines a character named Carol, with a short name and an image.


#### Step 6: Creating a Dialog Section

Next, let's create the corresponding dialog section. Add the following code to your script file:

```
=== my_dialog ===

carol: Hello! Welcome to my humble abode.
player: Hi, thank you for having me.

say: Say goodbye and leave.
act: end_conversation
```

This code defines a simple dialog section called "my_dialog" with a conversation between the player and Carol. The player has an option to say goodbye and leave the conversation.


#### Step 7: Ending the Conversation

Finally, let's create an action to end the conversation and return to the location. Add the following code to your script file:

```
=== end_conversation ===

carol: Goodbye! Have a great day!
act: my_location
```

This code defines an action called "end_conversation" that ends the conversation and returns the player to "my_location".


#### Step 8: Running Your First Yarn

Now that you've created your simple Yarnspin game script, save the "my_first_yarn.txt" file and run the Yarnspin compiler to generate the "yarnspin.dat" file. To play your game, simply launch the Yarnspin player.

Congratulations! You've just created your very first Yarnspin game. As you become more comfortable with the Yarnspin scripting language, you can start to add more locations, dialogs, characters, and interactions to

