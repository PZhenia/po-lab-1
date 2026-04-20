from locust import HttpUser, task, between

class WebServerUser(HttpUser):
    wait_time = between(1, 3)

    @task
    def get_index(self):
        self.client.get("/")

    @task
    def get_page2(self):
        self.client.get("/page2.html")

    @task
    def get_non_existent(self):
        with self.client.get("/unknown.html", catch_response=True) as response:
            if response.status_code == 404:
                response.success()