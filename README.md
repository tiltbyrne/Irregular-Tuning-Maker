# Irregular-Tuning-Maker
To understand the algorithm which Irregular Tuning Maker uses, begin with a scale $S$ which is made up of notes $N$ and intervals $I$. An interval to note $a$ from note $b$ is written $a \leftarrow b$. Naturally, $a \leftarrow b = \frac{1}{b \leftarrow a}$. You can think of the scale as a graph whose notes and intervals are nodes and edges respectively, i.e. $S = \left( N,I \right)$. A graph's edge can be weighted. Musically, the weight of an interval represents the relative importance of the accuracy of its tuning. For example, it is probably more important that the perfect fifth above a note is in tune than the minor ninth six octave above it, which implies $w\left( \text{perfect fifth} \right) > w\left( \text{minor ninth + six \textit{8va}} \right)$. The weight of an interval is commutative, meaning $w\left( a \leftarrow b \right) = w\left( b \leftarrow a \right)$, so $w\left( a \leftarrow b \right) = 1$. A tuning $T$ of a scale has $N$ notes. The note $r \in N$ is the root note of the scale, whose tuning, $t_{r} ≡ 1$. Where note $x \in N ≠ r$, it’s tuning $t_{x} = f_{N}\left( x \right)$.

$$f_{P}\left( x \right) = GM\left(\displaystyle \prod \limits_{p \in P}{g\left( x, p \right)^{\displaystyle \prod \limits_{q \in P}{w\left( p \leftarrow q \right)}}} \right)$$

$P$ contains the possible notes from which we could travel to note $x$ from. $GM$ stands for the geometric mean. The exponent of each base inside the product is itself the product of all intervals’ weights from the notes in $P$ to the note $p$. The sum of exponents whose reciprocal the product is raised to to make this geometric mean is therefore $\sum \limits_{q \in P}{\prod \limits_{q' \in P}{w\left( q \leftarrow q' \right) }}$. The function $g$ is defined as:

$$g\left( x, p \right) = \begin{cases}
  x \leftarrow r, & p = r \text{ or } p = x, \\
  x \leftarrow p \cdot f_{P - x} \left( p \right), & \text{otherwise}.
\end{cases}$$

A classic instructive example of the type of problem encountered in musical tuning is the three-note scale of $N\left( S \right) = \left(C , D, A\right)$. This scale contains the intervals $D \leftarrow C = \frac{9}{8}$, $A \leftarrow C = \frac{5}{3}$, and $A \leftarrow D = \frac{3}{2}$ which are called the major second, major sixth, and perfect fifth respectively. In this example $r = C$, $w\left( D \leftarrow C \right) = w\left( A \leftarrow C \right) = 1$ and $w\left( A \leftarrow D \right) = 2$. $t_D = 1.119... ≈ 195.3...\textcent$, and $t_A = 1.675... ≈ 893.0...\textcent \text{*}$.

$\text{*} t_D = f_{N}\left( D \right) = GM\left(\displaystyle \prod \limits_{n \in N}{g\left( D, n \right)^{\displaystyle \prod \limits_{q \in N}{w\left( n \leftarrow q \right)}}} \right) = \sqrt[5]{g\left(D, C\right)^1 \cdot g\left(D, D\right)^2 \cdot g\left(D, A\right)^2}$
$= \sqrt[5]{D \leftarrow C^1 \cdot D \leftarrow C^2 \cdot  \left(D \leftarrow A \cdot f_{N - D} \left( A \right) \right)^2}$
