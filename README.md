# Irregular-Tuning-Maker
Irregular Tuning Maker uses a novel algorithm to make tunings of any scale which has quantifiable ideal intervals between notes at any range, referred to as a scale space. Custom scale space files can easily be created and modified from within the app. By internally modelling the space as a weighted graph where edges represent ideal interval sizes and weights the relative importance of their accuracy in the resultant tuning, this program gives the user a high degree of control over the tuning process. Such customisability can be useful when a tuning could conceivably better approximate the ideal size of the intervals it includes with some note-nudging i.e. ad. hoc. tempering. Without a tool like this, it can be difficult to know where to begin, or indeed end, with such note-nudging, as evidenced by the multiplicity of Well Temperaments documented in seventeenth- and eighteenth-century Europe.

To get started, simply select/create a scale space, choose a weight function and range, press make, and a tuning will be created that the algorithm tries to ensure is musical. The degree to which the algorithm ‘tries’ is controlled by the precision dial. Once the calculation is finished, the scale is output in both ratios and cents.

![image alt](https://github.com/tiltbyrne/Irregular-Tuning-Maker/blob/main/READMEimage.png?raw=true)

User guide: https://youtu.be/deV8IO7XNw8

To understand the algorithm which Irregular Tuning Maker uses to make tunings, begin with a scale $S$ which is made up of notes $N$ and intervals $I$. Here, a scale does not just mean one octave of a scale, but every note of a scae within some range. An interval to note $a$ from note $b$ is written $a:b$. Naturally, $a:b = {b:a}^{-1}$ and $a:b:c =\left( a:b \right) \times \left( b:c \right)$. You can think of the scale as a graph whose notes and intervals are nodes and edges respectively, i.e. $S = \left( N,I \right)$. A graph's edge can be weighted. Musically, the weight of an interval represents the relative importance of the accuracy of its tuning. For example, it is probably more important that the perfect fifth above a note is in tune than the minor ninth six octave above it, which implies $w\left( \text{perfect fifth} \right) > w\left( \text{minor ninth + six \textit{8va}} \right)$. The weight of an interval is commutative, meaning $w\left( a:b \right) = w\left( b:a \right)$, so $w\left( a:a \right) = 1$. A tuning $T$ of a scale has $N$ notes. The note $r \in N$ is the root note of the scale, whose tuning, $t_{r} ≡ 1$. Where note $x \in N ≠ r$, it’s tuning $t_{x} = f_{N}\left( x \right)$.

$$f_{P}\left( x \right) = GM\left(\displaystyle \prod \limits_{p \in P}{g\left( x, p \right)^{\displaystyle \prod \limits_{q \in P}{w\left( p:q \right)}}} \right)$$

$P$ contains the possible notes from which we could travel to note $x$ from. $GM$ stands for the geometric mean. The exponent of each base inside the product is itself the product of all intervals’ weights from the notes in $P$ to the note $p$. The sum of exponents whose reciprocal the product is raised to to make this geometric mean is therefore $\sum \limits_{q \in P}{\prod \limits_{q' \in P}{w\left( q:q' \right) }}$. The function $g$ is defined as:

$$g\left( x, p \right) = \begin{cases}
  x:r, & p = r \text{ or } p = x, \\
  x:p f_{P - x} \left( p \right), & \text{otherwise}.
\end{cases}$$

A classic instructive example of the type of problem encountered in musical tuning is the three-note scale of $N\left( S \right) = \left(C , D, A\right)$. This scale contains the intervals $D:C = \frac{9}{8}$, $A:C = \frac{5}{3}$, and $A:D = \frac{3}{2}$ which are called the major second, major sixth, and perfect fifth respectively. In this example $r = C$, $w\left( D:C \right) = w\left( A:C \right) = 1$ and $w\left( A:D \right) = 2$. $t_D = 1.119\ldots≈195.3\ldots\textcent$, and $t_A = 1.675\ldots≈893.0\ldots\textcent$[^1].

By comparing the intervals $\mathrm{T}(I)$ to $I$ you can see a primary benefit of this way of tuning is that $w(I)$ is inversely proportional to Error $=$ $|\mathrm{T}(I) - I|$, how far $\mathrm{T}(I)$ is from $I$.

| Interval Name  | $I$           | $\mathrm{T}\left(I \right)$ | $w\left(I \right)$ | Error |
| -------------- | ------------- | ------------------------- | -------------------- | ----- |
| Major Second   | 203.9         | 195.3                     | 1                    | 8.6   |
| Perfect Fifth  | 702.0         | 697.7                     | 2                    | 4.3   |
| Major Sixth    | 884.4         | 893.0                     | 1                    | 8.6   |

Though clear relationships between scale input and resultant tuning break down for scales with more than three notes, thus hampering analysis of such tunings, any scale can theoretically be tuned like this. However, the real problem comes from the recursive structure of $f$ itself, since the number of calculations grows exponentially with the number of notes in the scale[^2]. In practice, the computational demands of tuning even as little as 24 notes would be impractical. So, recursion must be halted early. There are many ways you might do this, but the way I have chosen is to stop recursion when the total weight of an interval exceeds a lower cutoff $c$. First, $f$ is changed to take a second argument, which represents the total weight to which an individual interval is raised. Now, $t_n = f_N\left(x, 1\right)$ and

$$f_{P}\left( x , w \right) = \displaystyle \prod \limits_{p \in P}{g\left( x, p, w \cdot e_p \right)^{e_p}}$$

Since the exponent of each call to $g$ multiplied by the total weight $w$ is now passed as an additional third argument to $g$, that exponent is explicitly written as $e_p = \frac{\prod \limits_{p \in P}{w\left( p:q \right)}}{\sum \limits_{q \in P}{\prod \limits_{q' \in P}{w\left( q:q' \right) }}}$ for clarity, which is simply an alternative way of writing the geometric mean. Finally,

$$g\left( x, p, w \right) = \begin{cases}
  x:r, & p = r \text{ or } p = x \text{ or } w < c, \\
  x:p f_{P - x} \left( p, w \right), & \text{otherwise}.
\end{cases}$$

The value of $c$ is controlled by the precision dial in the application.

[^1]: $t_D = f_{N}\left( D \right) = GM\left(\displaystyle \prod \limits_{n \in N}{g\left( D, n \right)^{\displaystyle \prod \limits_{q \in N}{w\left( n:q \right)}}} \right) = \sqrt[5]{g\left(D, C\right)^1 \cdot g\left(D, D\right)^2 \cdot g\left(D, A\right)^2} = \sqrt[5]{D:C^1 \cdot D:C^2 \cdot \left(D:A \cdot f_{N - D} \left( A \right) \right)^2} f_{N - D} \left( A \right) = GM\left(g\left(A, C\right)^1 \cdot g\left(A, D\right)^1\right) = \sqrt[2]{A:C^1 \cdot A:C^1} = A:C t_D = \sqrt[5]{D:C^1 \cdot D:C^2 \cdot \left(D:A:C\right)^2} = \sqrt[5]{\left( \frac{9}{8} \cdot \frac{9}{8}^2 \cdot \left(\frac{2}{3} \cdot \frac{5}{3}\right)^2\right)} = 1.119\ldots. $t_A$ is calculated similarly.

[^2]: This is not the case if all weights are equal because when $w\left(a,b\right) = w\left(b,a\right)$ for any $a$ and $b$, $f_N\left(x \right) = \sqrt[N]{\prod \limits_{n \in N}{x:n}}$
