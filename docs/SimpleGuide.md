# A 5-Year-Old's Guide to Building a Language

Welcome, amazing language creator! We're going to build a secret language called **Omnikarai**.

Imagine you're building with LEGOs. You can't just stick them together any way you want, right? You have to follow some rules. Building a language is like that! We're creating the rules for how to write computer instructions.

Our language will be built in a special workshop called **C**. Think of C as the master LEGO set that lets us create our own, new kinds of LEGO blocks.

Let's start our adventure!

---

### Adventure 1: The Word Sorter (The Lexer)

Before we can build anything, we need to sort our LEGOs. Imagine you have a big box of blocks of different shapes and colors.

That's what the **Lexer** does! In our project, this is the file `src/lexer.c`.

When you write in Omnikarai like this:

```omnikarai
set my_toy = "robot"
```

The Lexer looks at it and sorts the pieces:
*   `set` -> "Hey, that's a special keyword!"
*   `my_toy` -> "That's a name for something!"
*   `=` -> "That's a symbol for giving a value!"
*   `"robot"` -> "That's a piece of text!"

The Lexer's job is to turn your code into a neat line of sorted blocks, which we call **tokens**. It's the very first step!

---

### Adventure 2: The Rule Checker (The Parser)

Now that we have our sorted LEGO blocks (tokens), we need to make sure they fit together correctly.

This is the job of the **Parser**! In our project, this is `src/parser.c`.

The Parser is like a rulebook. It checks if your instructions make sense.
*   `set my_toy = "robot"` -> **GOOD!** The rulebook says this is a correct sentence.
*   `set = "robot" my_toy` -> **BAD!** The rulebook says this is nonsense.

If the sentence is good, the Parser builds a **"blueprint"** of the instruction. This blueprint is called an **Abstract Syntax Tree (AST)**. It's just a way to show how all the pieces of your instruction are connected.

---

### Adventure 3: The Builder (The Interpreter/Compiler)

We have our sorted blocks and a blueprint. Now it's time to actually build something!

This is the job of the **Interpreter** or **Compiler**. In our project, `src/interpreter.c` and `src/compiler.c` do this.

The Interpreter is like a robot that follows your blueprint (the AST) to do things.
*   When it sees the blueprint for `set my_toy = "robot"`, it says: "Okay! I need to create a magic box called `my_toy` and put a `"robot"` inside it."

It actually *runs* your code and makes the computer do the work.

---

### How to Be a Language Creator Yourself!

So, how do you change or add things to the language? You just follow the same three steps!

**Let's say you want to create a new command called `shout` that prints things in BIG LETTERS.**

1.  **Teach the Lexer:**
    *   Go to `src/lexer.c`.
    *   Teach it to recognize the new word "shout" as a special keyword token.

2.  **Teach the Parser:**
    *   Go to `src/parser.c`.
    *   Teach it the new rule: a `shout` command should be followed by some text (e.g., `shout "hello"`).
    *   Make it build a new kind of blueprint (AST node) for your `shout` command.

3.  **Teach the Interpreter:**
    *   Go to `src/interpreter.c`.
    *   Teach it what to do when it sees the `shout` blueprint. It should take the text, make it uppercase, and print it to the screen.

### Summary: The Three Magic Steps

1.  **Lexer (The Word Sorter):** Turns code into a stream of tokens.
2.  **Parser (The Rule Checker):** Checks if the tokens form valid sentences and builds a blueprint (AST).
3.  **Interpreter (The Builder):** Follows the blueprint to run the code.

That's it! Every programming language, no matter how big or small, follows these same magic steps. Now you know the secret to building them. Happy creating!
