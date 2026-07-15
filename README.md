# Irregular-Tuning-Maker
This application provides a novel, user-friendly interface with which users can create irregular tunings of any scale. The goal of this approach to tuning is to provide the benefits that come with ad-hoc tunings, while giving the user tools to be explicit about what they are being ad-hoc. No deep understanding of tuning is required to use this method or application.

User guide: https://youtu.be/deV8IO7XNw8

To understand the algorithm which Irregular Tuning Maker uses to make tunings, begin with a scale $S$ which is made up of notes $N$ and intervals $I$. Here, a scale does not just mean one octave of a scale, but every note of a scae within some range. An interval to note $a$ from note $b$ is written $a \leftarrow b$. Naturally, $a \leftarrow b = \frac{1}{b \leftarrow a}$. You can think of the scale as a graph whose notes and intervals are nodes and edges respectively, i.e. $S = \left( N,I \right)$. A graph's edge can be weighted. Musically, the weight of an interval represents the relative importance of the accuracy of its tuning. For example, it is probably more important that the perfect fifth above a note is in tune than the minor ninth six octave above it, which implies $w\left( \text{perfect fifth} \right) > w\left( \text{minor ninth + six \textit{8va}} \right)$. The weight of an interval is commutative, meaning $w\left( a \leftarrow b \right) = w\left( b \leftarrow a \right)$, so $w\left( a \leftarrow b \right) = 1$. A tuning $T$ of a scale has $N$ notes. The note $r \in N$ is the root note of the scale, whose tuning, $t_{r} ≡ 1$. Where note $x \in N ≠ r$, it’s tuning $t_{x} = f_{N}\left( x \right)$.

$$f_{P}\left( x \right) = GM\left(\displaystyle \prod \limits_{p \in P}{g\left( x, p \right)^{\displaystyle \prod \limits_{q \in P}{w\left( p \leftarrow q \right)}}} \right)$$

$P$ contains the possible notes from which we could travel to note $x$ from. $GM$ stands for the geometric mean. The exponent of each base inside the product is itself the product of all intervals’ weights from the notes in $P$ to the note $p$. The sum of exponents whose reciprocal the product is raised to to make this geometric mean is therefore $\sum \limits_{q \in P}{\prod \limits_{q' \in P}{w\left( q \leftarrow q' \right) }}$. The function $g$ is defined as:

$$g\left( x, p \right) = \begin{cases}
  x \leftarrow r, & p = r \text{ or } p = x, \\
  x \leftarrow p \cdot f_{P - x} \left( p \right), & \text{otherwise}.
\end{cases}$$

A classic instructive example of the type of problem encountered in musical tuning is the three-note scale of $N\left( S \right) = \left(C , D, A\right)$. This scale contains the intervals $D \leftarrow C = \frac{9}{8}$, $A \leftarrow C = \frac{5}{3}$, and $A \leftarrow D = \frac{3}{2}$ which are called the major second, major sixth, and perfect fifth respectively. In this example $r = C$, $w\left( D \leftarrow C \right) = w\left( A \leftarrow C \right) = 1$ and $w\left( A \leftarrow D \right) = 2$. $t_D = 1.119... ≈ 195.3...\textcent$, and $t_A = 1.675... ≈ 893.0...\textcent ^{1}$.

By comparing the intervals $\text{T}(I)$ to $I$ you can see a primary benefit of this way of tuning is that $w(I)$ is inversely proportional to $\text{Error} = |\text{T}(I) - I|$, how far $\text{T}(I)$ is from $I$.

| Interval Name  | $I$           | $\text{T}\left(I \right)$ | $w\left(I \right)$ | Error |
| -------------- | ------------- | ------------------------- | ------------------ | ----- |
| Major Second   | 203.9         | 195.3                     | 1                  | 8.6   |
| Perfect Fifth  | 702.0         | 697.7                     | 2                  | 4.3   |
| Major Sixth    | 884.4         | 893.0                     | 1                  | 8.6   |

While clear relationships between scale input and resultant tuning breaks down for scales with more notes, thus hampering analysis, it is theoretically capable of tuning any scale. However, the calls to $f$ are recursive, meaning the number of calculation grows exponentially with the number of notes in the scale$^{2}$. In practice, the computational demands of tuning even a set of notes as little as 24 notes are impractical, meaning recursion must be halted early. There are many ways you might do this, but the way I have chosen is to stop recursion when the total weight of an interval exceeds a lower cutoff $c$. First, $f$ is changed to take a second argument, which represents the total weight to which an individual interval is raised. Now, $t_n = f_N\left(x, 1\right)$ and

$$f_{P}\left( x , w \right) = \displaystyle \prod \limits_{p \in P}{g\left( x, p, w \cdot e_p \right)^{e_p}}$$

Since the exponent of each call to $g$ multiplied by the total weight $w$ is now passed as an additional third argument to $g$, that exponent is explicitly written as $e_p = \frac{\prod \limits_{p \in P}{w\left( p \leftarrow q \right)}}{\sum \limits_{q \in P}{\prod \limits_{q' \in P}{w\left( q \leftarrow q' \right) }}}$ for clarity, which is simply an alternative way of writing the geometric mean. Finally,

$$g\left( x, p, w \right) = \begin{cases}
  x \leftarrow r, & p = r \text{ or } p = x \text{ or } w < c, \\
  x \leftarrow p \cdot f_{P - x} \left( p, w \right), & \text{otherwise}.
\end{cases}$$

The value of $c$ is controlled by the precision dial in the application.

1. $t_D = f_{N}\left( D \right)$
   $ = GM\left(\displaystyle \prod \limits_{n \in N}{g\left( D, n \right)^{\displaystyle \prod \limits_{q \in N}{w\left( n \leftarrow q \right)}}} \right) = \sqrt[5]{g\left(D, C\right)^1 \cdot g\left(D, D\right)^2 \cdot g\left(D, A\right)^2}$
$= \sqrt[5]{D \leftarrow C^1 \cdot D \leftarrow C^2 \cdot \left(D \leftarrow A \cdot f_{N - D} \left( A \right) \right)^2}$

$f_{N - D} \left( A \right) = GM\left(g\left(A, C\right)^1 \cdot g\left(A, D\right)^1\right) = \sqrt[2]{A \leftarrow C^1 \cdot A \leftarrow C^1} = A \leftarrow C$

$t_D = \sqrt[5]{D \leftarrow C^1 \cdot D \leftarrow C^2 \cdot \left(D \leftarrow A \leftarrow C\right)^2} = \sqrt[5]{\left( \frac{9}{8} \cdot \frac{9}{8}^2 \cdot \left(\frac{2}{3} \cdot \frac{5}{3}\right)^2\right)} = 1.119... ≈ 195.3...\textcent$

$t_A$ is calculated similarly.

2. This is not the case if all weights are equal because when $w\left(a,b\right) = w\left(b,a\right)$ for any $a$ and $b$, $f_N\left(x \right) = \sqrt[N]{\prod \limits_{n \in N}{x \leftarrow n}}$
