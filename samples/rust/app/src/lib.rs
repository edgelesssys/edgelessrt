// https://doc.rust-lang.org/stable/rust-by-example/std_misc/threads.html

use std::thread;

static NTHREADS: i32 = 10;

// This is the `main` thread
#[no_mangle]
extern "C" fn rustmain() {
    // Make a vector to hold the children which are spawned.
    let mut children = vec![];

    for i in 0..NTHREADS {
        // Spin up another thread
        children.push(thread::spawn(move || {
            println!("this is thread number {}", i);
        }));
    }

    for child in children {
        // Wait for the thread to finish. Returns a result.
        let _ = child.join();
    }
}
