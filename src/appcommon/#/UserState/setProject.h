// #memfn
void UserState::setProject(Project project) {
    project = std::move(project);
}
// #defneeds: utility