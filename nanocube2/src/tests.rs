use cube::Cube;
use cube::Monoid;
use naivecube::Naivecube;
use nanocube;
use nanocube::Nanocube;
use rand;
use rand::Rng;
use rand::distributions::Uniform;
use std::ops::Range;
use std;
use std::cmp;
use timeit;

fn generate_random_dataset(widths: &Vec<usize>, n: usize) -> Vec<Vec<usize>> {
    let mut rng = rand::thread_rng();
    let mut result = Vec::new();
    let dims: Vec<Range<usize>> = widths
        .iter()
        .map(|w| (0..(1 as usize) << w))
        .collect();
    for _ in 0..n {
        result.push(dims.iter().map(|r| {
            let dist = Uniform::new(r.start, r.end);
            rng.sample(dist)
        }).collect());
    }
    result
}

fn generate_random_ranges(widths: &Vec<usize>, n: usize) -> Vec<Vec<(usize, usize)>> {
    let mut rng = rand::thread_rng();
    let mut result = Vec::new();
    let dims: Vec<Range<usize>> = widths
        .iter()
        .map(|w| (0..(1 as usize) << w))
        .collect();
    for _ in 0..n {
        result.push(
            dims.iter()
                .map(|r| {
                    let dist = Uniform::new(r.start, r.end);
                    let v1 = rng.sample(dist);
                    let v2 = rng.sample(dist);
                    (cmp::min(v1, v2), cmp::max(v1, v2) + 1)
                })
                .collect(),
        );
    }
    result
}

fn partition_data(data: &[Vec<usize>], n: usize) -> Vec<&[Vec<usize>]> {
    let mut result = Vec::new();

    for i in 0..n {
        let start = data.len() * i / n;
        let end = data.len() * (i + 1) / n;
        result.push(&data[start..end]);
    }

    debug_assert!(result.len() == n);
    result
}

#[test]
pub fn nanocube_is_equivalent_to_naivecube() {
    // let nruns = 100;
    // let width = vec![2,3];
    // let npoints = 2;
    // let nranges = 5;
    let nruns = 100;
    let width = vec![24, 2, 2];
    let npoints = 100;
    let nranges = 5;

    let nloops = 1;
    let sec = timeit_loops!(nloops, {
        for _ in 0..nruns {
            let data = generate_random_dataset(&width, npoints);
            let mut naivecube = Naivecube::<usize>::new(width.clone());
            let mut nanocube = Nanocube::<usize>::new(width.clone());

            let mut summaries = Vec::new();
            // println!("data:");
            // println!("{:?}", data);
            for point in &data {
                naivecube.add(1, point);
                summaries.push(1);
            }
            nanocube.add_many(&summaries, &data);

            let ranges = generate_random_ranges(&width, nranges);

            for range in ranges {
                let naive_result = naivecube.range_query(&range);
                let nc_result = nanocube.range_query(&range);
                if naive_result != nc_result {
                    println!("mismatch!");
                    println!("data: {:?}", &data);
                    println!("query region: {:?}", &range);
                    println!("naive result: {:?}", &naive_result);
                    println!("nanocube res: {:?}", &nc_result);
                    nanocube::write_dot_to_disk("out/bad_nc.dot", &nanocube)
                        .expect("internal error");
                    println!("{:?}", nanocube.summaries.values);
                }
            }
        }
    });

    println!(
        "{} insertions in {} secs (rate: {}).\n",
        nloops * nruns * npoints,
        (nloops as f64) * sec,
        ((nruns * npoints) as f64) / sec
    );
}

#[test]
pub fn parallel_nanocube_is_equivalent_to_nanocube() {
    // in this test, we build a number of nanocubes "in parallel" by
    // splitting the input vector, and then merge all of them in a final pass.

    let width = vec![3];
    let npoints = 100;

    let data = generate_random_dataset(&width, npoints);
    let ncubes = 3;

    let mut nanocubes: Vec<Nanocube<usize>> = partition_data(&data, ncubes)
        .iter()
        .map(|slice| {
            let summaries: Vec<usize> = (0..slice.len()).map(|_x| 1).collect();
            let mut result = Nanocube::new_from_many(width.clone(), &summaries, slice);
            result.flush_release_list();
            result
        })
        .collect();

    nanocube::write_dot_to_disk("out/nc1.dot", &nanocubes[0]).expect("Couldn't write");
    nanocube::write_dot_to_disk("out/nc2.dot", &nanocubes[1]).expect("Couldn't write");
    nanocube::write_dot_to_disk("out/nc3.dot", &nanocubes[2]).expect("Couldn't write");
    // nanocube::write_dot_to_disk("out/nc4.dot", &nanocubes[3]).expect("Couldn't write");

    let mut final_nanocube = nanocubes[0].clone();

    for (i, d) in nanocubes.iter().enumerate() {
        if i == 0 {
            continue;
        }
        final_nanocube = final_nanocube.mapply(d);
    };

    // let nanocubes = Nanocube::

    let mut naivecube = Naivecube::<usize>::new(width.clone());
    let mut summaries = Vec::new();
    for point in &data {
        naivecube.add(1, point);
        summaries.push(1);
    }
    
    let nranges = 5;
    let ranges = generate_random_ranges(&width, nranges);

    for range in ranges {
        let naive_result = naivecube.range_query(&range);
        let nc_result = final_nanocube.range_query(&range);
        if naive_result != nc_result {
            println!("mismatch!");
            println!("data: {:?}", &data);
            println!("query region: {:?}", &range);
            println!("naive result: {:?}", &naive_result);
            println!("nanocube res: {:?}", &nc_result);
            nanocube::write_dot_to_disk("out/bad_nc.dot", &final_nanocube)
                .expect("internal error");
            println!("{:?}", final_nanocube.summaries.values);
        }
    }
}
